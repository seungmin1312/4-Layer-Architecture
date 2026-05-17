#include "ddi_mutex.h"
#include "../drv/drv_rtos.h"
#include "../tools/common.h"

/* FSP common_data generates these TX_MUTEX objects at boot. We keep their
 * names matched to the slot enum entries so the dispatch is a clean switch. */
extern TX_MUTEX g_mutex_system_info;
extern TX_MUTEX g_mutex_adcfilter_info;

static TX_MUTEX* get_handle(ddi_mutex_slot_e slot)
{
    switch (slot) {
        case E_DDI_MUTEX_SYSTEM_INFO:
            return &g_mutex_system_info;
        case E_DDI_MUTEX_ADCFILTER_INFO:
            return &g_mutex_adcfilter_info;
        default:
            return NULL;
    }
}

static ULONG ms_to_ticks(uint32_t timeoutMs)
{
    ULONG ticks = 0UL;

    if (timeoutMs == D_DDI_MUTEX_NO_WAIT) {
        return TX_NO_WAIT;
    }
    if (timeoutMs == D_DDI_MUTEX_WAIT_FOREVER) {
        return TX_WAIT_FOREVER;
    }

    ticks = (timeoutMs * TX_TIMER_TICKS_PER_SECOND) / 1000U;
    /* Sub-tick timeouts round up to 1 tick to avoid accidental NO_WAIT. */
    if (ticks == 0UL) {
        ticks = 1UL;
    }
    return ticks;
}

fw_status_t ddi_mutex_get(ddi_mutex_slot_e slot, uint32_t timeoutMs)
{
    TX_MUTEX* mutex  = get_handle(slot);
    UINT      status = 0U;

    if (mutex == NULL) {
        error("ddi_mutex_get: invalid slot=%d", slot);
        return FW_FAIL;
    }
    status = tx_mutex_get(mutex, ms_to_ticks(timeoutMs));
    if (status != TX_SUCCESS) {
        error("ddi_mutex_get fail: slot=%d, tx_status=%u", slot, (unsigned)status);
        return FW_FAIL;
    }
    return FW_OK;
}

fw_status_t ddi_mutex_put(ddi_mutex_slot_e slot)
{
    TX_MUTEX* mutex = get_handle(slot);

    if (mutex == NULL) {
        error("ddi_mutex_put: invalid slot=%d", slot);
        return FW_FAIL;
    }
    return (tx_mutex_put(mutex) == TX_SUCCESS) ? FW_OK : FW_FAIL;
}
