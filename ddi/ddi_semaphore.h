#ifndef DDI_SEMAPHORE_H_
#define DDI_SEMAPHORE_H_

#include "../tools/fw_types.h"

/**
 * @file ddi_semaphore.h
 * @brief Slot-enum abstraction over ThreadX TX_SEMAPHORE.
 *
 * Same rationale as ddi_mutex (layer isolation, ms conversion, single fail
 * log point), with one critical difference:
 *
 *   - `ddi_semaphore_put` is **ISR-safe** - this is the main wakeup channel
 *     from ISR top-half to RTOS task bottom-half.
 *   - `ddi_semaphore_get` is task-only when timeout != 0. In ISR context,
 *     only NO_WAIT (zero timeout) is permitted.
 */

#define D_DDI_SEM_NO_WAIT       (0U)
#define D_DDI_SEM_WAIT_FOREVER  (0xFFFFFFFFU)

typedef enum {
    E_DDI_SEMAPHORE_ADC_CTRL_DONE,                      /* AGT1 ISR -> th_safety_ctrl */
    E_DDI_SEMAPHORE_ADC_BASE_DONE,                      /* AGT1 ISR -> th_acq         */
    E_DDI_SEMAPHORE_UART_RX_READY,                      /* DTC ISR  -> th_comm        */
    E_DDI_SEMAPHORE_CONSOLE_RX_READY,                   /* SCI0 ISR -> th_console     */
    E_DDI_SEMAPHORE_HANDLE_MAX,
    E_DDI_SEMAPHORE_HANDLE_UNKNOWN = E_DDI_SEMAPHORE_HANDLE_MAX
} ddi_semaphore_slot_e;

fw_status_t ddi_semaphore_get(ddi_semaphore_slot_e slot, uint32_t timeoutMs);
fw_status_t ddi_semaphore_put(ddi_semaphore_slot_e slot); /* ISR-safe */

#endif /* DDI_SEMAPHORE_H_ */
