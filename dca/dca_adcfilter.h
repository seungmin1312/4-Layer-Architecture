#ifndef DCA_ADCFILTER_H_
#define DCA_ADCFILTER_H_

#include "../tools/common.h"
#include "../ddi/ddi_adc.h"
#include "../ddi/ddi_dmac.h"
#include "../ddi/ddi_timer.h"

/* Filter buffer sizes (concrete values used in this project; safe to publish). */
#define D_CTRL_FILTER_SIZE      (10U)
#define D_BASE_FILTER_SIZE      (100U)
#define D_BATTERY_FILTER_SIZE   (274U)

#define D_BASE_TRIM_COUNT       (5U)        /* trim top/bottom 5 samples before averaging */
#define D_CTRL_TRIM_COUNT       (0U)

#define D_ADC_VREF_MILLIVOLT    (3300U)
#define D_ADC_RESOLUTION        (12U)

typedef struct {
    bool          isRun;
    bool          displayEnable;
    volatile bool fullFlag;                             /* set by DMAC ISR, cleared by task */
} dca_adcfilter_flag_t;

typedef enum {
    /* Control-loop (DMAC-fed) channels - sampled in sync with PWM */
    E_DCA_FILTER_CH_A,
    E_DCA_FILTER_CH_B,
    E_DCA_FILTER_CH_C,
    E_DCA_FILTER_CH_D,
    /* Base (slow) channels - polled at 10ms cadence via register read */
    E_DCA_FILTER_CH_SLOW_START,
    E_DCA_FILTER_CH_SLOW_0 = E_DCA_FILTER_CH_SLOW_START,
    E_DCA_FILTER_CH_SLOW_1,
    E_DCA_FILTER_CH_SLOW_2,
    E_DCA_FILTER_CH_SLOW_3,
    E_DCA_FILTER_CH_SLOW_4,
    E_DCA_FILTER_CH_SLOW_5,
    E_DCA_FILTER_MAX,
    E_DCA_FILTER_UNKNOWN = E_DCA_FILTER_MAX
} dca_adcfilter_slot_e;

typedef struct {
    uint16_t* buf;
    size_t    size;
    uint8_t   trimCnt;
    uint16_t  result;
} dca_adcfilter_rawbuf_info_t;

/* Cross-thread-shared filtered values. Read via dca_adcfilter_get_snapshot(). */
typedef struct {
    uint16_t chAMv;
    uint16_t chBMv;
    uint16_t chCMv;
    uint16_t chDMv;
    uint16_t slow0Mv;
    uint16_t slow1Mv;
    /* additional fields redacted (domain-specific) */
} dca_adcfilter_info_t;

/* Internal pointer - same-thread, no lock. Cross-thread reads must use the
 * snapshot copy below. */
dca_adcfilter_info_t* dca_adcfilter_get_info(void);

/* Mutex-protected snapshot copy. Safe to call from any task. */
fw_status_t dca_adcfilter_get_snapshot(dca_adcfilter_info_t* out);

void        dca_adcfilter_init(void);
fw_status_t dca_adcfilter_enable (void);
fw_status_t dca_adcfilter_disable(void);

/* Called from task context (th_safety_ctrl) once per ISR semaphore signal.
 * Walks DMAC channels and applies the filter to any channel whose buffer
 * is full. ISR-set `fullFlag` is the only shared state. */
void dca_adcfilter_run_ctrl_loop(void);

/* Called from task context (th_acq) once per 10ms tick. Reads one sample
 * per slow channel; once the buffer is full, applies trimmed-mean filter. */
void dca_adcfilter_run_base_tick   (void);
void dca_adcfilter_run_battery_tick(void);

#endif /* DCA_ADCFILTER_H_ */
