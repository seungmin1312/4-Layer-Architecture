#ifndef TH_SAFETY_CTRL_ENTRY_H_
#define TH_SAFETY_CTRL_ENTRY_H_

#include "../tools/common.h"
#include "../dca/dca_system.h"
#include "../dca/dca_adcfilter.h"
#include "../ddi/ddi_semaphore.h"

/* th_safety_ctrl: highest-cadence control / safety thread.
 * Wakes every 1ms via semaphore put from AGT1 ISR. */
void app_th_safety_ctrl_entry(void);

#endif /* TH_SAFETY_CTRL_ENTRY_H_ */
