#ifndef TH_ACQ_ENTRY_H_
#define TH_ACQ_ENTRY_H_

#include "../tools/common.h"
#include "../dca/dca_adcfilter.h"
#include "../ddi/ddi_semaphore.h"

/* th_acq: ADC slow-path acquisition thread.
 * Wakes every 10ms via semaphore put from AGT1 ISR. */
void app_th_acq_entry(void);

#endif /* TH_ACQ_ENTRY_H_ */
