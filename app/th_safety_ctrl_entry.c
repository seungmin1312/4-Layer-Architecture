#include "th_safety_ctrl_entry.h"

/* ===========================================================================
 * th_safety_ctrl - 1 ms safety / control thread
 *
 * Wake semantics
 * --------------
 * AGT1 ISR puts E_DDI_SEMAPHORE_ADC_CTRL_DONE once per 1 ms. This loop
 * blocks on that semaphore with a 5 ms fallback - if the ISR ever stops
 * signaling, the safety tasks still run, just at degraded cadence.
 *
 * Run-loop body: a deterministic, ordered sequence of safety-relevant tasks.
 * The order matters - GPIO must be polled before fault evaluation, which
 * must run before any RF-output state machine decision (domain logic for
 * the latter is redacted in this portfolio).
 * ========================================================================= */

#define D_TH_SAFETY_FALLBACK_TICK_MS  (5U)

void app_th_safety_ctrl_entry(void)
{
#if D_USE_TH_SAFETY_CTRL
    for (;;) {
        (void)ddi_semaphore_get(E_DDI_SEMAPHORE_ADC_CTRL_DONE,
                                D_TH_SAFETY_FALLBACK_TICK_MS);

        /* (1) Debounced GPIO poll - the only place inputs are sampled.
         *     Edges fan out to registered DCA handlers via callback. */
        dca_system_poll_all_inputs();

        /* (2) DMAC-fed ADC filter loop - process any buffer-full channels
         *     and update the cross-thread snapshot. */
        dca_adcfilter_run_ctrl_loop();

        /* (3) Domain-specific safety state machine call sites are redacted
         *     in this portfolio. In the real project they go here:
         *     - integrated fault evaluation
         *     - RF-output state machine step
         *     - RF source service (DAC update, pre-shot, etc.) */
    }
#else
    return;
#endif
}
