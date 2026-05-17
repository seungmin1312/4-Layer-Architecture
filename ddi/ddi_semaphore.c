#include "ddi_semaphore.h"
#include "../drv/drv_rtos.h"
#include "../tools/common.h"

extern TX_SEMAPHORE g_sem_adc_ctrl_done;
extern TX_SEMAPHORE g_sem_adc_base_done;
extern TX_SEMAPHORE g_sem_uart_rx_ready;
extern TX_SEMAPHORE g_sem_console_rx_ready;

static TX_SEMAPHORE* get_handle(ddi_semaphore_slot_e slot)
{
    switch (slot) {
        case E_DDI_SEMAPHORE_ADC_CTRL_DONE:
            return &g_sem_adc_ctrl_done;
        case E_DDI_SEMAPHORE_ADC_BASE_DONE:
            return &g_sem_adc_base_done;
        case E_DDI_SEMAPHORE_UART_RX_READY:
            return &g_sem_uart_rx_ready;
        case E_DDI_SEMAPHORE_CONSOLE_RX_READY:
            return &g_sem_console_rx_ready;
        default:
            return NULL;
    }
}

static ULONG ms_to_ticks(uint32_t timeoutMs)
{
    ULONG ticks = 0UL;

    if (timeoutMs == D_DDI_SEM_NO_WAIT) {
        return TX_NO_WAIT;
    }
    if (timeoutMs == D_DDI_SEM_WAIT_FOREVER) {
        return TX_WAIT_FOREVER;
    }

    ticks = (timeoutMs * TX_TIMER_TICKS_PER_SECOND) / 1000U;
    if (ticks == 0UL) {
        ticks = 1UL;
    }
    return ticks;
}

fw_status_t ddi_semaphore_get(ddi_semaphore_slot_e slot, uint32_t timeoutMs)
{
    TX_SEMAPHORE* sem    = get_handle(slot);
    UINT          status = 0U;

    if (sem == NULL) {
        error("ddi_semaphore_get: invalid slot=%d", slot);
        return FW_FAIL;
    }

    status = tx_semaphore_get(sem, ms_to_ticks(timeoutMs));
    if (status != TX_SUCCESS) {
        /* TX_NO_INSTANCE on NO_WAIT is normal flow ("no signal yet"), so we
         * skip the error log to avoid drowning the trace buffer. */
        if (status != TX_NO_INSTANCE) {
            error("ddi_semaphore_get fail: slot=%d, tx_status=%u", slot, (unsigned)status);
        }
        return FW_FAIL;
    }
    return FW_OK;
}

/* ISR-safe. Note we do NOT call error() here - RTT logging from ISR causes
 * unpredictable latency that defeats the purpose of a short top-half. */
fw_status_t ddi_semaphore_put(ddi_semaphore_slot_e slot)
{
    TX_SEMAPHORE* sem = get_handle(slot);

    if (sem == NULL) {
        return FW_FAIL;
    }
    return (tx_semaphore_put(sem) == TX_SUCCESS) ? FW_OK : FW_FAIL;
}
