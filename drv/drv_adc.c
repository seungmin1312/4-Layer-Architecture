#include "drv_adc.h"
#include "../tools/common.h"

extern const adc_instance_t    g_adc0;
extern const adc_instance_t    g_adc1;
extern const adc_channel_cfg_t g_adc0_channel_cfg;
extern const adc_channel_cfg_t g_adc1_channel_cfg;

static const drv_adc_handle_t adcHandle[E_DRV_ADC_HANDLE_MAX] = {
    { &g_adc0, &g_adc0_channel_cfg },
    { &g_adc1, &g_adc1_channel_cfg }
};

static drv_adc_callback_t cbAdc0ScanDone = NULL;

static const drv_adc_handle_t* drv_adc_get_handle(drv_adc_slot_e slot)
{
    if (slot >= E_DRV_ADC_HANDLE_MAX) {
        error("Invalid ADC slot=%d", slot);
        return NULL;
    }
    return &adcHandle[slot];
}

fw_status_t drv_adc_open(drv_adc_slot_e slot)
{
    const drv_adc_handle_t* handle = drv_adc_get_handle(slot);

    if (handle == NULL) {
        return FW_FAIL;
    }
    return (R_ADC_Open(handle->instance->p_ctrl, handle->instance->p_cfg)
            == FSP_SUCCESS) ? FW_OK : FW_FAIL;
}

fw_status_t drv_adc_close(drv_adc_slot_e slot)
{
    const drv_adc_handle_t* handle = drv_adc_get_handle(slot);

    if (handle == NULL) {
        return FW_FAIL;
    }
    return (R_ADC_Close(handle->instance->p_ctrl) == FSP_SUCCESS) ? FW_OK : FW_FAIL;
}

fw_status_t drv_adc_scan_cfg(drv_adc_slot_e slot)
{
    const drv_adc_handle_t* handle = drv_adc_get_handle(slot);

    if (handle == NULL) {
        return FW_FAIL;
    }
    return (R_ADC_ScanCfg(handle->instance->p_ctrl, handle->channelCfg)
            == FSP_SUCCESS) ? FW_OK : FW_FAIL;
}

fw_status_t drv_adc_scan_start(drv_adc_slot_e slot)
{
    const drv_adc_handle_t* handle = drv_adc_get_handle(slot);

    if (handle == NULL) {
        return FW_FAIL;
    }
    return (R_ADC_ScanStart(handle->instance->p_ctrl) == FSP_SUCCESS) ? FW_OK : FW_FAIL;
}

fw_status_t drv_adc_scan_stop(drv_adc_slot_e slot)
{
    const drv_adc_handle_t* handle = drv_adc_get_handle(slot);

    if (handle == NULL) {
        return FW_FAIL;
    }
    return (R_ADC_ScanStop(handle->instance->p_ctrl) == FSP_SUCCESS) ? FW_OK : FW_FAIL;
}

/* Register-direct read for the slow-path ADC1 channels.
 * (FSP's R_ADC_Read works too, but a direct ADDR[] read avoids the per-call
 *  validation overhead for the safety-loop critical path.) */
fw_status_t drv_adc_get_register_data(drv_adc_channel_slot_e slot, uint16_t* out)
{
    if ((slot >= E_DRV_ADC_CHANNEL_MAX) || (out == NULL)) {
        return FW_FAIL;
    }

    switch (slot) {
        case E_DRV_ADC_1_CHANNEL_0:
            *out = R_ADC1->ADDR[0];
            break;
        case E_DRV_ADC_1_CHANNEL_18:
            *out = R_ADC1->ADDR[18];
            break;
        case E_DRV_ADC_1_CHANNEL_19:
            *out = R_ADC1->ADDR[19];
            break;
        case E_DRV_ADC_1_CHANNEL_20:
            *out = R_ADC1->ADDR[20];
            break;
        case E_DRV_ADC_1_CHANNEL_21:
            *out = R_ADC1->ADDR[21];
            break;
        case E_DRV_ADC_1_CHANNEL_22:
            *out = R_ADC1->ADDR[22];
            break;
        default:
            return FW_FAIL;
    }
    return FW_OK;
}

fw_status_t drv_adc_init(drv_adc_slot_e slot)
{
    if (drv_adc_open(slot) != FW_OK) {
        return FW_FAIL;
    }
    if (drv_adc_scan_cfg(slot) != FW_OK) {
        return FW_FAIL;
    }
    return FW_OK;
}

/* ===========================================================================
 * ISR entry + callback registration (ADC0 only - ADC1 is polled register-read)
 * ========================================================================= */

void isr_adc0_trg(adc_callback_args_t* pArgs)
{
    if (pArgs->event == ADC_EVENT_SCAN_COMPLETE) {
        if (cbAdc0ScanDone != NULL) {
            cbAdc0ScanDone();
        }
    }
}

void drv_adc_0_register_callback(drv_adc_callback_t cb)
{
    cbAdc0ScanDone = cb;
}
