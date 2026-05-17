#include "ddi_adc.h"
#include "../tools/common.h"

/* This file is the textbook "thin wrapper" - it adds slot validation and a
 * single error-logging point, then forwards 1:1 to drv_adc. The reason it
 * exists at all is dependency direction: the DCA layer must compile without
 * ever pulling in r_adc_api.h, so we re-export a vendor-neutral API here. */

fw_status_t ddi_adc_open(ddi_adc_slot_e slot)
{
    if (slot >= E_DDI_ADC_HANDLE_MAX) {
        error("Invalid ADC slot=%d", slot);
        return FW_FAIL;
    }
    return drv_adc_open((drv_adc_slot_e)slot);
}

fw_status_t ddi_adc_close(ddi_adc_slot_e slot)
{
    if (slot >= E_DDI_ADC_HANDLE_MAX) {
        return FW_FAIL;
    }
    return drv_adc_close((drv_adc_slot_e)slot);
}

fw_status_t ddi_adc_scan_cfg(ddi_adc_slot_e slot)
{
    if (slot >= E_DDI_ADC_HANDLE_MAX) {
        return FW_FAIL;
    }
    return drv_adc_scan_cfg((drv_adc_slot_e)slot);
}

fw_status_t ddi_adc_scan_start(ddi_adc_slot_e slot)
{
    if (slot >= E_DDI_ADC_HANDLE_MAX) {
        return FW_FAIL;
    }
    return drv_adc_scan_start((drv_adc_slot_e)slot);
}

fw_status_t ddi_adc_scan_stop(ddi_adc_slot_e slot)
{
    if (slot >= E_DDI_ADC_HANDLE_MAX) {
        return FW_FAIL;
    }
    return drv_adc_scan_stop((drv_adc_slot_e)slot);
}

fw_status_t ddi_adc_get_register_data(ddi_adc_channel_slot_e slot, uint16_t* out)
{
    if (slot >= E_DDI_ADC_CHANNEL_MAX) {
        return FW_FAIL;
    }
    return drv_adc_get_register_data((drv_adc_channel_slot_e)slot, out);
}

fw_status_t ddi_adc_init(ddi_adc_slot_e slot)
{
    if (slot >= E_DDI_ADC_HANDLE_MAX) {
        return FW_FAIL;
    }
    return drv_adc_init((drv_adc_slot_e)slot);
}

/* Callback registration forwarder - same function-pointer cast trick keeps
 * the upper layer free of any drv_* typedefs. */
void ddi_adc_0_handle(ddi_adc_callback_t cb)
{
    drv_adc_0_register_callback((drv_adc_callback_t)cb);
}
