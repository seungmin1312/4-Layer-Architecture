#ifndef DRV_TIMER_H_
#define DRV_TIMER_H_

#include "r_timer_api.h"
#include "r_gpt.h"
#include "r_agt.h"

#include "../tools/fw_types.h"

typedef void (*drv_timer_callback_t)(void);

/* GPT slots 0..7, AGT slots 8..9. Specific frequencies / wiring is redacted -
 * upper layers refer to slots by name only via the ddi_timer enum. */
typedef enum {
    E_DRV_TIMER_0,                                      /* GPT0 - usage redacted */
    E_DRV_TIMER_1,                                      /* GPT1 - ADC0 trigger via ELC */
    E_DRV_TIMER_2,                                      /* GPT2 */
    E_DRV_TIMER_3,                                      /* GPT3 */
    E_DRV_TIMER_4,                                      /* GPT4 */
    E_DRV_TIMER_5,                                      /* GPT5 */
    E_DRV_TIMER_6,                                      /* GPT6 */
    E_DRV_TIMER_7,                                      /* GPT7 */
    E_DRV_TIMER_8,                                      /* AGT0 */
    E_DRV_TIMER_9,                                      /* AGT1 - 1ms tick + ADC1 trig */
    E_DRV_TIMER_HANDLE_MAX,
    E_DRV_TIMER_HANDLE_UNKNOWN = E_DRV_TIMER_HANDLE_MAX
} drv_timer_slot_e;

#define D_GPT_MAX_PERCENT          (100U)
#define D_FREQUENCY_UNITS_KHZ      (1000ULL)

fw_status_t drv_timer_open (drv_timer_slot_e slot);
fw_status_t drv_timer_start(drv_timer_slot_e slot);
fw_status_t drv_timer_stop (drv_timer_slot_e slot);
fw_status_t drv_timer_reset(drv_timer_slot_e slot);
fw_status_t drv_timer_close(drv_timer_slot_e slot);
fw_status_t drv_timer_set_duty_cycle(drv_timer_slot_e slot, uint8_t dutyPercent);
fw_status_t drv_timer_set_frequency (drv_timer_slot_e slot, uint32_t frequencyKhz);
uint32_t    drv_timer_get_tick(void);
fw_status_t drv_timer_init (drv_timer_slot_e slot);

/* Direct GTCCRD write for sub-period offset (e.g. delayed ADC trigger). */
fw_status_t drv_timer_gpt_set_compare_match_d_delay_us(drv_timer_slot_e slot, uint32_t delayUs);

void drv_timer_agt0_register_callback(drv_timer_callback_t cb);
void drv_timer_agt1_register_callback(drv_timer_callback_t cb);

#endif /* DRV_TIMER_H_ */
