#include "drv_timer.h"
#include "../tools/common.h"

/* FSP timer instances. Original code suffixes carry RF-specific meaning;
 * we leave the variable names neutral here. */
extern const timer_instance_t g_timer0;
extern const timer_instance_t g_timer1;
extern const timer_instance_t g_timer2;
extern const timer_instance_t g_timer3;
extern const timer_instance_t g_timer4;
extern const timer_instance_t g_timer5;
extern const timer_instance_t g_timer6;
extern const timer_instance_t g_timer7;
extern const timer_instance_t a_timer0;                 /* AGT0 */
extern const timer_instance_t a_timer1;                 /* AGT1 - 1ms tick + ADC1 trigger */

static volatile uint32_t tick1ms = 0U;

static const timer_instance_t* timerHandle[E_DRV_TIMER_HANDLE_MAX] = {
    &g_timer0, &g_timer1, &g_timer2, &g_timer3,
    &g_timer4, &g_timer5, &g_timer6, &g_timer7,
    &a_timer0, &a_timer1
};

static drv_timer_callback_t cbAgt0 = NULL;
static drv_timer_callback_t cbAgt1 = NULL;

static const timer_instance_t* drv_timer_get_handle(drv_timer_slot_e slot)
{
    if (slot >= E_DRV_TIMER_HANDLE_MAX) {
        error("Invalid Timer slot=%d", slot);
        return NULL;
    }
    return timerHandle[slot];
}

/* Helper: GPT vs AGT dispatch.
 * Slots 0..7 are GPT, slots 8..9 are AGT. The dispatch table approach keeps
 * the caller-facing API uniform regardless of which IP block is used. */
static bool is_agt(drv_timer_slot_e slot)
{
    return (slot >= E_DRV_TIMER_8);
}

fw_status_t drv_timer_open(drv_timer_slot_e slot)
{
    const timer_instance_t* handle = drv_timer_get_handle(slot);

    if (handle == NULL) {
        return FW_FAIL;
    }
    if (is_agt(slot)) {
        return (R_AGT_Open(handle->p_ctrl, handle->p_cfg) == FSP_SUCCESS) ? FW_OK : FW_FAIL;
    }
    return (R_GPT_Open(handle->p_ctrl, handle->p_cfg) == FSP_SUCCESS) ? FW_OK : FW_FAIL;
}

fw_status_t drv_timer_start(drv_timer_slot_e slot)
{
    const timer_instance_t* handle = drv_timer_get_handle(slot);
    fsp_err_t               err    = FSP_SUCCESS;

    if (handle == NULL) {
        return FW_FAIL;
    }
    err = is_agt(slot) ? R_AGT_Start(handle->p_ctrl) : R_GPT_Start(handle->p_ctrl);
    return (err == FSP_SUCCESS) ? FW_OK : FW_FAIL;
}

fw_status_t drv_timer_stop(drv_timer_slot_e slot)
{
    const timer_instance_t* handle = drv_timer_get_handle(slot);
    fsp_err_t               err    = FSP_SUCCESS;

    if (handle == NULL) {
        return FW_FAIL;
    }
    err = is_agt(slot) ? R_AGT_Stop(handle->p_ctrl) : R_GPT_Stop(handle->p_ctrl);
    return (err == FSP_SUCCESS) ? FW_OK : FW_FAIL;
}

fw_status_t drv_timer_reset(drv_timer_slot_e slot)
{
    const timer_instance_t* handle = drv_timer_get_handle(slot);
    fsp_err_t               err    = FSP_SUCCESS;

    if (handle == NULL) {
        return FW_FAIL;
    }
    err = is_agt(slot) ? R_AGT_Reset(handle->p_ctrl) : R_GPT_Reset(handle->p_ctrl);
    return (err == FSP_SUCCESS) ? FW_OK : FW_FAIL;
}

fw_status_t drv_timer_close(drv_timer_slot_e slot)
{
    const timer_instance_t* handle = drv_timer_get_handle(slot);
    fsp_err_t               err    = FSP_SUCCESS;

    if (handle == NULL) {
        return FW_FAIL;
    }
    err = is_agt(slot) ? R_AGT_Close(handle->p_ctrl) : R_GPT_Close(handle->p_ctrl);
    return (err == FSP_SUCCESS) ? FW_OK : FW_FAIL;
}

fw_status_t drv_timer_set_duty_cycle(drv_timer_slot_e slot, uint8_t dutyPercent)
{
    const timer_instance_t* handle  = drv_timer_get_handle(slot);
    timer_info_t            info    = { 0 };
    uint32_t                dutyCnt = 0U;

    if (dutyPercent > D_GPT_MAX_PERCENT) {
        return FW_FAIL;
    }
    if (handle == NULL) {
        return FW_FAIL;
    }
    if (R_GPT_InfoGet(handle->p_ctrl, &info) != FSP_SUCCESS) {
        return FW_FAIL;
    }

    dutyCnt = (uint32_t)(((uint64_t)info.period_counts * dutyPercent) / D_GPT_MAX_PERCENT);
    return (R_GPT_DutyCycleSet(handle->p_ctrl, dutyCnt, GPT_IO_PIN_GTIOCA) == FSP_SUCCESS)
            ? FW_OK : FW_FAIL;
}

fw_status_t drv_timer_set_frequency(drv_timer_slot_e slot, uint32_t frequencyKhz)
{
    const timer_instance_t* handle    = drv_timer_get_handle(slot);
    uint32_t                clkHz     = 0U;
    uint32_t                targetHz  = 0U;
    uint32_t                periodCnt = 0U;

    if (handle == NULL) {
        return FW_FAIL;
    }

    clkHz     = R_FSP_SystemClockHzGet(FSP_PRIV_CLOCK_PCLKA) >> (uint32_t)handle->p_cfg->source_div;
    targetHz  = (uint32_t)((uint64_t)frequencyKhz * D_FREQUENCY_UNITS_KHZ);
    periodCnt = clkHz / targetHz;

    if (R_GPT_PeriodSet(handle->p_ctrl, periodCnt) != FSP_SUCCESS) {
        return FW_FAIL;
    }
    /* default 50% duty after period change */
    return (R_GPT_DutyCycleSet(handle->p_ctrl, periodCnt / 2U, GPT_IO_PIN_GTIOCA) == FSP_SUCCESS)
            ? FW_OK : FW_FAIL;
}

uint32_t drv_timer_get_tick(void)
{
    return tick1ms;
}

/* Direct GTCCRD write to schedule a sub-period ELC trigger offset.
 *
 * In the original project this offsets ADC0 sampling so that we sample at a
 * specific point inside each PWM cycle. FSP doesn't expose GTCCRD via its
 * API, so we write the register directly under GTWP write-protect bracketing.
 *
 * This is the one place where vendor register access leaks below the
 * FSP-API line - it is intentionally isolated to the DRV layer.
 */
fw_status_t drv_timer_gpt_set_compare_match_d_delay_us(drv_timer_slot_e slot, uint32_t delayUs)
{
    const timer_instance_t* handle = NULL;
    timer_info_t            info   = { 0 };
    uint64_t                offset = 0ULL;
    uint32_t                wpPrev = 0U;

    if (is_agt(slot)) {
        return FW_FAIL;
    }
    handle = drv_timer_get_handle(slot);
    if (handle == NULL) {
        return FW_FAIL;
    }
    if (R_GPT_InfoGet(handle->p_ctrl, &info) != FSP_SUCCESS) {
        return FW_FAIL;
    }

    offset = ((uint64_t)info.clock_frequency * (uint64_t)delayUs) / 1000000ULL;
    if (offset < 1ULL) {
        offset = 1ULL;
    }
    if (offset >= (uint64_t)info.period_counts) {
        offset = (uint64_t)info.period_counts - 1ULL;
    }

    /* GTWP unlock + GTCCRD write + relock. GTCCR[4] is the FSP mapping of GTCCRD. */
    wpPrev = R_GPT1->GTWP;
    R_GPT1->GTWP     = 0xA500U;
    R_GPT1->GTCCR[4] = (uint32_t)(offset - 1ULL);
    R_GPT1->GTWP     = (wpPrev | 0xA500U);

    return FW_OK;
}

fw_status_t drv_timer_init(drv_timer_slot_e slot)
{
    fw_status_t status = drv_timer_open(slot);

    if ((slot == E_DRV_TIMER_9) && (status == FW_OK)) {
        tick1ms = 0U;
    }
    return status;
}

/* ===========================================================================
 * AGT ISR entry points (1 ms tick on AGT1)
 *
 * AGT1's CYCLE_END is the 1ms heartbeat for the whole system. It also acts
 * as the ELC trigger source for ADC1 scans, so the upper-layer callback
 * registered here is what wakes the safety_ctrl thread.
 * ========================================================================= */

void isr_agt0_adc_trg(timer_callback_args_t* pArgs)
{
    if ((pArgs->event == TIMER_EVENT_CYCLE_END) && (cbAgt0 != NULL)) {
        cbAgt0();
    }
}

void isr_agt1_adc_trg(timer_callback_args_t* pArgs)
{
    if (pArgs->event == TIMER_EVENT_CYCLE_END) {
        if (cbAgt1 != NULL) {
            cbAgt1();
        }
        tick1ms++;                                      /* monotonic 1ms counter */
    }
}

void drv_timer_agt0_register_callback(drv_timer_callback_t cb)
{
    cbAgt0 = cb;
}

void drv_timer_agt1_register_callback(drv_timer_callback_t cb)
{
    cbAgt1 = cb;
}
