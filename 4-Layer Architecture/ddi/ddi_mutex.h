#ifndef DDI_MUTEX_H_
#define DDI_MUTEX_H_

#include "../tools/fw_types.h"

/**
 * @file ddi_mutex.h
 * @brief Slot-enum abstraction over ThreadX TX_MUTEX.
 *
 * Why an enum wrapper instead of exposing TX_MUTEX* directly?
 *
 *   1. **Layer isolation** - DCA / APP never see tx_api.h. The RTOS could
 *      be swapped for FreeRTOS, RT-Thread, etc. by editing only ddi_mutex.c.
 *
 *   2. **ms->tick conversion in one place** - callers pass plain milliseconds.
 *
 *   3. **Single fail-logging point** - all mutex failures are logged with
 *      the same format string and same context.
 *
 *   4. **Compile-time no-op** - setting D_USE_DDI_MUTEX=0 in common.h turns
 *      all mutex calls into no-ops at the caller site (see helper macros in
 *      dca/dca_*.c).
 *
 * **ISR usage prohibited** - these functions call tx_mutex_get/put which
 * are not ISR-safe. Use ddi_semaphore from ISR context.
 */

#define D_DDI_MUTEX_NO_WAIT       (0U)
#define D_DDI_MUTEX_WAIT_FOREVER  (0xFFFFFFFFU)

typedef enum {
    E_DDI_MUTEX_SYSTEM_INFO,                            /* protects dca_system_info_t */
    E_DDI_MUTEX_ADCFILTER_INFO,                         /* protects dca_adcfilter_info_t */
    E_DDI_MUTEX_HANDLE_MAX,
    E_DDI_MUTEX_HANDLE_UNKNOWN = E_DDI_MUTEX_HANDLE_MAX
} ddi_mutex_slot_e;

fw_status_t ddi_mutex_get(ddi_mutex_slot_e slot, uint32_t timeoutMs); /* task only */
fw_status_t ddi_mutex_put(ddi_mutex_slot_e slot);

#endif /* DDI_MUTEX_H_ */
