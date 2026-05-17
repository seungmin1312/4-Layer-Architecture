#include "dca_adcfilter.h"
#include "../ddi/ddi_mutex.h"
#include "../ddi/ddi_semaphore.h"

/* ===========================================================================
 * dca_adcfilter - the centerpiece of the portfolio
 *
 * This module demonstrates the ISR top-half / task bottom-half split for a
 * high-rate sampled signal path. Four DMAC channels each fire ~per PWM cycle;
 * each ISR is exactly one line. All filtering, mV conversion and shared-state
 * update happens in task context (th_safety_ctrl).
 *
 *   PWM period    ELC trigger    ADC0 scan    DMAC fills buf
 *   -----------------------------------------------------------
 *   GPT1 cycle ----> ADC0 -------> DMAC[0..3] ----> ISR: fullFlag = true
 *                                                       (one line)
 *                                                              |
 *                                                              v
 *   AGT1 1ms tick -> sem put -> th_safety_ctrl wakes -> run_ctrl_loop()
 *                                                       (filter + snapshot)
 *
 * Mutex protection is via the slot-enum wrapper; lock window is just the
 * struct-copy in `get_snapshot()` so contention is microseconds at most.
 * ========================================================================= */

static dca_adcfilter_info_t adcfilterInfo;

/* Compile-time switchable mutex (no-op when D_USE_DDI_MUTEX = 0). */
static inline void lock_adcfilter_info(void)
{
#if D_USE_DDI_MUTEX
    (void)ddi_mutex_get(E_DDI_MUTEX_ADCFILTER_INFO, D_DDI_MUTEX_WAIT_FOREVER);
#endif
}

static inline void unlock_adcfilter_info(void)
{
#if D_USE_DDI_MUTEX
    (void)ddi_mutex_put(E_DDI_MUTEX_ADCFILTER_INFO);
#endif
}

dca_adcfilter_info_t* dca_adcfilter_get_info(void)
{
    return &adcfilterInfo;
}

fw_status_t dca_adcfilter_get_snapshot(dca_adcfilter_info_t* out)
{
    if (out == NULL) {
        return FW_FAIL;
    }
    lock_adcfilter_info();
    memcpy(out, &adcfilterInfo, sizeof(*out));          /* tiny lock window */
    unlock_adcfilter_info();
    return FW_OK;
}

/* ===========================================================================
 * ISR top-half (running in ADC DMAC interrupt context)
 *
 * Note how short these are. The whole point is to leave the ISR within
 * microseconds and let the task handle the math. `volatile` on fullFlag
 * is what allows the task to safely poll without a memory barrier.
 * ========================================================================= */

static dca_adcfilter_flag_t filterFlag[E_DCA_FILTER_MAX] = { { 0 } };

static void on_dmac0_done(void)
{
    filterFlag[E_DCA_FILTER_CH_A].fullFlag = true;
}

static void on_dmac1_done(void)
{
    filterFlag[E_DCA_FILTER_CH_B].fullFlag = true;
}

static void on_dmac2_done(void)
{
    filterFlag[E_DCA_FILTER_CH_C].fullFlag = true;
}

static void on_dmac3_done(void)
{
    filterFlag[E_DCA_FILTER_CH_D].fullFlag = true;
}

/* AGT1 fires every 1ms. We use this as the master tick:
 *   - every 1ms  -> wake th_safety_ctrl
 *   - every 10ms -> wake th_acq for slow-channel sampling
 *
 * Still one line per branch. The 10ms counter is the only state. */
static volatile uint32_t adcTick1ms  = 0U;
static volatile uint32_t adcTick10ms = 0U;

static void on_agt1_tick_1ms(void)
{
#if D_USE_TH_SAFETY_CTRL
    (void)ddi_semaphore_put(E_DDI_SEMAPHORE_ADC_CTRL_DONE);
#endif
    adcTick1ms++;
    if (adcTick1ms >= 10U) {
        adcTick1ms = 0U;
        adcTick10ms++;
#if D_USE_TH_ACQ
        (void)ddi_semaphore_put(E_DDI_SEMAPHORE_ADC_BASE_DONE);
#endif
    }
}

/* ===========================================================================
 * Filter helpers - pure functions, called from task context only
 * ========================================================================= */

static uint16_t filter_trimmed_mean(uint16_t* buf, size_t size, uint8_t trimCnt)
{
    static uint16_t sorted[D_BATTERY_FILTER_SIZE];
    uint16_t        temp  = 0U;
    uint32_t        sum   = 0U;
    uint16_t        count = 0U;
    size_t          i     = 0U;
    size_t          j     = 0U;

    if ((buf == NULL) || (size == 0U) || (size > D_BATTERY_FILTER_SIZE)) {
        return 0U;
    }
    memcpy(sorted, buf, size * sizeof(uint16_t));

    /* O(N^2) sort - adequate for buffers <= 300 samples and avoids qsort overhead. */
    for (i = 0U; (i + 1U) < size; i++) {
        for (j = i + 1U; j < size; j++) {
            if (sorted[i] > sorted[j]) {
                temp      = sorted[i];
                sorted[i] = sorted[j];
                sorted[j] = temp;
            }
        }
    }

    for (i = trimCnt; (i + trimCnt) < size; i++) {
        sum += sorted[i];
        count++;
    }
    return (count > 0U) ? (uint16_t)(sum / count) : 0U;
}

static uint16_t filter_average(uint16_t* buf, size_t size)
{
    uint32_t sum = 0U;
    size_t   i   = 0U;

    if ((buf == NULL) || (size == 0U)) {
        return 0U;
    }
    for (i = 0U; i < size; i++) {
        sum += buf[i];
    }
    return (uint16_t)(sum / size);
}

static uint16_t adc_to_voltage_mv(uint16_t adc, uint16_t vref, uint8_t resolution)
{
    uint32_t maxAdc = 0U;

    if ((resolution == 0U) || (resolution > 16U)) {
        return 0U;
    }
    maxAdc = (1UL << resolution) - 1UL;
    return (uint16_t)(((uint32_t)adc * vref + (maxAdc / 2U)) / maxAdc);
}

/* ===========================================================================
 * Buffer wiring - one set of static buffers per channel
 * ========================================================================= */

static uint16_t bufChA  [D_CTRL_FILTER_SIZE]    = { 0 };
static uint16_t bufChB  [D_CTRL_FILTER_SIZE]    = { 0 };
static uint16_t bufChC  [D_CTRL_FILTER_SIZE]    = { 0 };
static uint16_t bufChD  [D_CTRL_FILTER_SIZE]    = { 0 };
static uint16_t bufSlow0[D_BASE_FILTER_SIZE]    = { 0 };
static uint16_t bufSlow1[D_BASE_FILTER_SIZE]    = { 0 };
static uint16_t bufSlow2[D_BASE_FILTER_SIZE]    = { 0 };
static uint16_t bufSlow3[D_BATTERY_FILTER_SIZE] = { 0 };
static uint16_t bufSlow4[D_BASE_FILTER_SIZE]    = { 0 };
static uint16_t bufSlow5[D_BASE_FILTER_SIZE]    = { 0 };

static dca_adcfilter_rawbuf_info_t rawBufInfo[E_DCA_FILTER_MAX] = {
    { bufChA,   D_CTRL_FILTER_SIZE,    D_CTRL_TRIM_COUNT, 0 },
    { bufChB,   D_CTRL_FILTER_SIZE,    D_CTRL_TRIM_COUNT, 0 },
    { bufChC,   D_CTRL_FILTER_SIZE,    D_CTRL_TRIM_COUNT, 0 },
    { bufChD,   D_CTRL_FILTER_SIZE,    D_CTRL_TRIM_COUNT, 0 },
    { bufSlow0, D_BASE_FILTER_SIZE,    D_BASE_TRIM_COUNT, 0 },
    { bufSlow1, D_BASE_FILTER_SIZE,    D_BASE_TRIM_COUNT, 0 },
    { bufSlow2, D_BASE_FILTER_SIZE,    D_BASE_TRIM_COUNT, 0 },
    { bufSlow3, D_BATTERY_FILTER_SIZE, D_BASE_TRIM_COUNT, 0 },
    { bufSlow4, D_BASE_FILTER_SIZE,    D_BASE_TRIM_COUNT, 0 },
    { bufSlow5, D_BASE_FILTER_SIZE,    D_BASE_TRIM_COUNT, 0 },
};

static uint16_t filteredMilliVoltage[E_DCA_FILTER_MAX] = { 0 };

/* Domain-specific mapping from filtered value -> info struct field.
 * Real mapping involves calibration tables - those are redacted here. */
static void update_adcfilter_variable(dca_adcfilter_slot_e slot, uint16_t mv)
{
    dca_adcfilter_info_t* info = dca_adcfilter_get_info();

    switch (slot) {
        case E_DCA_FILTER_CH_A:
            info->chAMv = mv;
            break;
        case E_DCA_FILTER_CH_B:
            info->chBMv = mv;
            break;
        case E_DCA_FILTER_CH_C:
            info->chCMv = mv;
            break;
        case E_DCA_FILTER_CH_D:
            info->chDMv = mv;
            break;
        case E_DCA_FILTER_CH_SLOW_0:
            info->slow0Mv = mv;
            break;
        case E_DCA_FILTER_CH_SLOW_1:
            info->slow1Mv = mv;
            break;
        /* additional cases redacted - calibration / domain-specific */
        default:
            break;
    }
}

/* ===========================================================================
 * Bottom-half: run from th_safety_ctrl
 * ========================================================================= */

void dca_adcfilter_run_ctrl_loop(void)
{
    dca_adcfilter_slot_e slot = E_DCA_FILTER_CH_A;

    /* For each control-loop channel, if the DMAC ISR signaled buffer-full,
     * apply the filter and re-arm the DMAC. */
    for (slot = E_DCA_FILTER_CH_A; slot < E_DCA_FILTER_CH_SLOW_START; slot++) {
        if (filterFlag[slot].fullFlag == false) {
            continue;
        }
        filterFlag[slot].fullFlag = false;

        if (filterFlag[slot].isRun) {
            rawBufInfo[slot].result = filter_average(rawBufInfo[slot].buf, rawBufInfo[slot].size);
            filteredMilliVoltage[slot] =
                adc_to_voltage_mv(rawBufInfo[slot].result, D_ADC_VREF_MILLIVOLT, D_ADC_RESOLUTION);
            update_adcfilter_variable(slot, filteredMilliVoltage[slot]);

            if (ddi_dmac_reconfigure((ddi_dmac_slot_e)slot) != FW_OK) {
                error("DMAC reconfigure fail slot=%d", slot);
                return;
            }
        } else {
            (void)ddi_dmac_disable((ddi_dmac_slot_e)slot);
        }
    }
}

/* Slow-path tick: one sample per channel per 10 ms call. */
void dca_adcfilter_run_base_tick(void)
{
    static uint16_t        idx     = 0U;
    dca_adcfilter_slot_e   slot    = E_DCA_FILTER_CH_SLOW_START;
    ddi_adc_channel_slot_e channel = E_DDI_ADC_1_CHANNEL_0;

    for (slot = E_DCA_FILTER_CH_SLOW_START; slot < E_DCA_FILTER_MAX; slot++) {
        if (slot == E_DCA_FILTER_CH_SLOW_3) {
            continue;                                   /* battery handled separately */
        }
        channel = (ddi_adc_channel_slot_e)(slot - E_DCA_FILTER_CH_SLOW_START);
        if (ddi_adc_get_register_data(channel, &rawBufInfo[slot].buf[idx]) != FW_OK) {
            return;
        }
    }
    idx++;

    if (idx >= D_BASE_FILTER_SIZE) {
        for (slot = E_DCA_FILTER_CH_SLOW_START; slot < E_DCA_FILTER_MAX; slot++) {
            if (slot == E_DCA_FILTER_CH_SLOW_3) {
                continue;
            }
            rawBufInfo[slot].result = filter_trimmed_mean(
                rawBufInfo[slot].buf, rawBufInfo[slot].size, rawBufInfo[slot].trimCnt);
            filteredMilliVoltage[slot] =
                adc_to_voltage_mv(rawBufInfo[slot].result, D_ADC_VREF_MILLIVOLT, D_ADC_RESOLUTION);
            update_adcfilter_variable(slot, filteredMilliVoltage[slot]);
        }
        idx = 0U;
    }
}

void dca_adcfilter_run_battery_tick(void)
{
    static uint16_t            idx     = 0U;
    const dca_adcfilter_slot_e slot    = E_DCA_FILTER_CH_SLOW_3;
    ddi_adc_channel_slot_e     channel = (ddi_adc_channel_slot_e)(slot - E_DCA_FILTER_CH_SLOW_START);

    if (ddi_adc_get_register_data(channel, &rawBufInfo[slot].buf[idx]) != FW_OK) {
        return;
    }
    idx++;

    if (idx >= D_BATTERY_FILTER_SIZE) {
        rawBufInfo[slot].result = filter_trimmed_mean(
            rawBufInfo[slot].buf, rawBufInfo[slot].size, rawBufInfo[slot].trimCnt);
        filteredMilliVoltage[slot] =
            adc_to_voltage_mv(rawBufInfo[slot].result, D_ADC_VREF_MILLIVOLT, D_ADC_RESOLUTION);
        update_adcfilter_variable(slot, filteredMilliVoltage[slot]);
        idx = 0U;
    }
}

/* ===========================================================================
 * Init - sequence is documented in the original source as a hard requirement
 * by the FSP / ELC wiring (ADC open -> DMAC open -> AGT open -> ELC).
 * Registering ISR callbacks last is intentional: it guarantees no callback
 * can fire before its handler exists.
 * ========================================================================= */

void dca_adcfilter_init(void)
{
    if (ddi_adc_init(E_DDI_ADC_0) != FW_OK) {
        error("ADC0 init fail");
        return;
    }
    if (ddi_adc_init(E_DDI_ADC_1) != FW_OK) {
        error("ADC1 init fail");
        return;
    }

    if (ddi_dmac_init(E_DDI_DMAC_0, bufChA) != FW_OK) {
        error("DMAC0 init fail");
        return;
    }
    if (ddi_dmac_init(E_DDI_DMAC_1, bufChB) != FW_OK) {
        error("DMAC1 init fail");
        return;
    }
    if (ddi_dmac_init(E_DDI_DMAC_2, bufChC) != FW_OK) {
        error("DMAC2 init fail");
        return;
    }
    if (ddi_dmac_init(E_DDI_DMAC_3, bufChD) != FW_OK) {
        error("DMAC3 init fail");
        return;
    }

    if (ddi_timer_init(E_DDI_TIMER_9) != FW_OK) {
        error("AGT1 init fail");
        return;
    }

    /* Offset ADC0 sampling inside the PWM period (concrete delay redacted -
     * real value is timed against the customer's RF gate waveform). */
    if (ddi_timer_gpt_set_compare_match_d_delay_us(E_DDI_TIMER_1, /* delay_us */ 0U) != FW_OK) {
        error("Compare-match D set fail");
        return;
    }

    /* Register ISR callbacks last - upward dispatch into this module. */
    ddi_dmac_0_handle(on_dmac0_done);
    ddi_dmac_1_handle(on_dmac1_done);
    ddi_dmac_2_handle(on_dmac2_done);
    ddi_dmac_3_handle(on_dmac3_done);
    ddi_timer_agt1_handle(on_agt1_tick_1ms);
}

fw_status_t dca_adcfilter_enable(void)
{
    /* Start sequence: ADC scan -> AGT tick.
     * DMAC is enabled lazily per-channel inside run_ctrl_loop on demand. */
    if (ddi_adc_scan_start(E_DDI_ADC_0) != FW_OK) {
        return FW_FAIL;
    }
    if (ddi_adc_scan_start(E_DDI_ADC_1) != FW_OK) {
        return FW_FAIL;
    }
    if (ddi_timer_start(E_DDI_TIMER_9) != FW_OK) {
        return FW_FAIL;
    }
    return FW_OK;
}

fw_status_t dca_adcfilter_disable(void)
{
    (void)ddi_adc_scan_stop(E_DDI_ADC_0);
    (void)ddi_adc_scan_stop(E_DDI_ADC_1);
    return FW_OK;
}
