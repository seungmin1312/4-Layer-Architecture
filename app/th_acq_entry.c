#include "th_acq_entry.h"

/* ===========================================================================
 * th_acq - slow-channel ADC acquisition thread
 *
 * Wake semantics
 * --------------
 * - AGT1 ISR calls `ddi_semaphore_put(E_DDI_SEMAPHORE_ADC_BASE_DONE)` every
 *   10ms via dca_adcfilter::on_agt1_tick_1ms (10ms branch).
 * - This loop waits on that semaphore with a 20ms fallback timeout.
 *
 * Why the fallback? If the ISR ever misses a signal (e.g. higher-priority
 * preemption holds longer than one cycle), the timeout still lets the
 * acquisition loop tick. The system degrades to "polling at 20ms" instead
 * of stopping.
 * ========================================================================= */

#define D_TH_ACQ_FALLBACK_TICK_MS  (20U)

void app_th_acq_entry(void)
{
#if D_USE_TH_ACQ
    for (;;) {
        /* Block until ISR signals or fallback timeout fires. */
        (void)ddi_semaphore_get(E_DDI_SEMAPHORE_ADC_BASE_DONE,
                                D_TH_ACQ_FALLBACK_TICK_MS);

        /* One semaphore signal == one tick == one sample per slow channel.
         * Filtering is applied internally once the per-channel buffer is full. */
        dca_adcfilter_run_base_tick();
        dca_adcfilter_run_battery_tick();
    }
#else
    /* Thread object not created at FSP level when this is 0 - so this
     * function is never called. The early return is a belt-and-braces. */
    return;
#endif
}
