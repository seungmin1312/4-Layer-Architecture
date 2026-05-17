#include "th_comm_entry.h"

/* ===========================================================================
 * th_comm - UART packet RX/TX dispatch thread
 *
 * Wake semantics
 * --------------
 * Two reasons this thread wakes:
 *
 *   (a) UART RX semaphore set by `on_rx_complete_dtc` (ISR) when a DTC
 *       block has been copied to the ring buffer.
 *   (b) Periodic timeout (150 ms) - even with no RX activity, we still
 *       run app_comm_tx_task() so the peer sees a heartbeat.
 *
 * The single semaphore + timeout pattern collapses both wakeup sources into
 * one wait call. The flag distinguishing "RX woke me" vs "timeout woke me"
 * is the return value of ddi_semaphore_get itself.
 *
 * Forward declarations of the actual TX/RX tasks are kept abstract here -
 * concrete protocol logic lives in app_txrxcom.c (redacted).
 * ========================================================================= */

extern void app_comm_rx_task(void);                     /* defined in app_txrxcom.c (redacted) */
extern void app_comm_tx_task(void);

void app_th_comm_entry(void)
{
#if D_USE_TH_COMM
    fw_status_t rxSignaled = FW_FAIL;

    for (;;) {
        /* Block until ISR signals RX, or fallback timeout. */
        rxSignaled = ddi_semaphore_get(E_DDI_SEMAPHORE_UART_RX_READY, D_TX_PERIOD_TIME_DEF);

        /* DTC watchdog poll - always run, regardless of why we woke. */
        dca_serial_poll();

        if (rxSignaled == FW_OK) {
            /* RX path: parse anything in the ring buffer. */
            app_comm_rx_task();
        } else {
            /* Timeout path: periodic TX heartbeat. */
            app_comm_tx_task();
        }
    }
#else
    return;
#endif
}
