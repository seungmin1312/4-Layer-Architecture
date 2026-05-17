#ifndef TH_COMM_ENTRY_H_
#define TH_COMM_ENTRY_H_

#include "../tools/common.h"
#include "../dca/dca_serial.h"
#include "../ddi/ddi_semaphore.h"

/* TX period - if no RX event for this long, send a heartbeat TX packet. */
#define D_TX_PERIOD_TIME_DEF  (150U)

/* th_comm: LCD packet comm thread.
 * Wakes on UART RX semaphore or periodic timeout. */
void app_th_comm_entry(void);

#endif /* TH_COMM_ENTRY_H_ */
