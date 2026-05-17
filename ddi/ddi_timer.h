#ifndef DDI_TIMER_H_
#define DDI_TIMER_H_

#include "../tools/fw_types.h"
#include "../drv/drv_timer.h"

typedef void (*ddi_timer_callback_t)(void);

typedef enum {
    E_DDI_TIMER_0,
    E_DDI_TIMER_1,                                      /* ADC0 trigger via ELC compare-match D */
    E_DDI_TIMER_2,
    E_DDI_TIMER_3,
    E_DDI_TIMER_4,
    E_DDI_TIMER_5,
    E_DDI_TIMER_6,
    E_DDI_TIMER_7,
    E_DDI_TIMER_8,                                      /* AGT0 */
    E_DDI_TIMER_9,                                      /* AGT1 - 1 ms base tick */
    E_DDI_TIMER_HANDLE_MAX,
    E_DDI_TIMER_HANDLE_UNKNOWN = E_DDI_TIMER_HANDLE_MAX
} ddi_timer_slot_e;

fw_status_t ddi_timer_start (ddi_timer_slot_e slot);
fw_status_t ddi_timer_stop  (ddi_timer_slot_e slot);
fw_status_t ddi_timer_reset (ddi_timer_slot_e slot);
fw_status_t ddi_timer_close (ddi_timer_slot_e slot);
fw_status_t ddi_timer_set_duty_cycle(ddi_timer_slot_e slot, uint8_t dutyPercent);
fw_status_t ddi_timer_set_frequency (ddi_timer_slot_e slot, uint32_t frequencyKhz);
uint32_t    ddi_timer_get_tick(void);
fw_status_t ddi_timer_gpt_set_compare_match_d_delay_us(ddi_timer_slot_e slot, uint32_t delayUs);
fw_status_t ddi_timer_init (ddi_timer_slot_e slot);

void ddi_timer_agt0_handle(ddi_timer_callback_t cb);
void ddi_timer_agt1_handle(ddi_timer_callback_t cb);

#endif /* DDI_TIMER_H_ */
