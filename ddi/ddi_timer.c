#include "ddi_timer.h"
#include "../tools/common.h"

fw_status_t ddi_timer_start(ddi_timer_slot_e slot)
{
    if (slot >= E_DDI_TIMER_HANDLE_MAX) {
        return FW_FAIL;
    }
    return drv_timer_start((drv_timer_slot_e)slot);
}

fw_status_t ddi_timer_stop(ddi_timer_slot_e slot)
{
    if (slot >= E_DDI_TIMER_HANDLE_MAX) {
        return FW_FAIL;
    }
    return drv_timer_stop((drv_timer_slot_e)slot);
}

fw_status_t ddi_timer_reset(ddi_timer_slot_e slot)
{
    if (slot >= E_DDI_TIMER_HANDLE_MAX) {
        return FW_FAIL;
    }
    return drv_timer_reset((drv_timer_slot_e)slot);
}

fw_status_t ddi_timer_close(ddi_timer_slot_e slot)
{
    if (slot >= E_DDI_TIMER_HANDLE_MAX) {
        return FW_FAIL;
    }
    return drv_timer_close((drv_timer_slot_e)slot);
}

fw_status_t ddi_timer_set_duty_cycle(ddi_timer_slot_e slot, uint8_t dutyPercent)
{
    if (slot >= E_DDI_TIMER_HANDLE_MAX) {
        return FW_FAIL;
    }
    return drv_timer_set_duty_cycle((drv_timer_slot_e)slot, dutyPercent);
}

fw_status_t ddi_timer_set_frequency(ddi_timer_slot_e slot, uint32_t frequencyKhz)
{
    if (slot >= E_DDI_TIMER_HANDLE_MAX) {
        return FW_FAIL;
    }
    return drv_timer_set_frequency((drv_timer_slot_e)slot, frequencyKhz);
}

uint32_t ddi_timer_get_tick(void)
{
    return drv_timer_get_tick();
}

fw_status_t ddi_timer_gpt_set_compare_match_d_delay_us(ddi_timer_slot_e slot, uint32_t delayUs)
{
    if (slot >= E_DDI_TIMER_HANDLE_MAX) {
        return FW_FAIL;
    }
    return drv_timer_gpt_set_compare_match_d_delay_us((drv_timer_slot_e)slot, delayUs);
}

fw_status_t ddi_timer_init(ddi_timer_slot_e slot)
{
    if (slot >= E_DDI_TIMER_HANDLE_MAX) {
        return FW_FAIL;
    }
    return drv_timer_init((drv_timer_slot_e)slot);
}

void ddi_timer_agt0_handle(ddi_timer_callback_t cb)
{
    drv_timer_agt0_register_callback(cb);
}

void ddi_timer_agt1_handle(ddi_timer_callback_t cb)
{
    drv_timer_agt1_register_callback(cb);
}
