#include "drv_dmac.h"
#include "../tools/common.h"

extern const transfer_instance_t g_transfer0;
extern const transfer_instance_t g_transfer1;
extern const transfer_instance_t g_transfer2;
extern const transfer_instance_t g_transfer3;

static const transfer_instance_t* transferHandle[E_DRV_DMAC_HANDLE_MAX] = {
    &g_transfer0, &g_transfer1, &g_transfer2, &g_transfer3
};

static drv_dmac_callback_t cbDmac[E_DRV_DMAC_HANDLE_MAX] = { NULL, NULL, NULL, NULL };

static const transfer_instance_t* drv_transfer_get_handle(drv_dmac_slot_e slot)
{
    if (slot >= E_DRV_DMAC_HANDLE_MAX) {
        error("Invalid DMAC slot=%d", slot);
        return NULL;
    }
    return transferHandle[slot];
}

fw_status_t drv_dmac_open(drv_dmac_slot_e slot)
{
    const transfer_instance_t* handle = drv_transfer_get_handle(slot);

    if (handle == NULL) {
        return FW_FAIL;
    }
    return (R_DMAC_Open(handle->p_ctrl, handle->p_cfg) == FSP_SUCCESS) ? FW_OK : FW_FAIL;
}

fw_status_t drv_dmac_close(drv_dmac_slot_e slot)
{
    const transfer_instance_t* handle = drv_transfer_get_handle(slot);

    if (handle == NULL) {
        return FW_FAIL;
    }
    return (R_DMAC_Close(handle->p_ctrl) == FSP_SUCCESS) ? FW_OK : FW_FAIL;
}

fw_status_t drv_dmac_enable(drv_dmac_slot_e slot)
{
    const transfer_instance_t* handle = drv_transfer_get_handle(slot);

    if (handle == NULL) {
        return FW_FAIL;
    }
    return (R_DMAC_Enable(handle->p_ctrl) == FSP_SUCCESS) ? FW_OK : FW_FAIL;
}

fw_status_t drv_dmac_disable(drv_dmac_slot_e slot)
{
    const transfer_instance_t* handle = drv_transfer_get_handle(slot);

    if (handle == NULL) {
        return FW_FAIL;
    }
    return (R_DMAC_Disable(handle->p_ctrl) == FSP_SUCCESS) ? FW_OK : FW_FAIL;
}

fw_status_t drv_dmac_reconfigure(drv_dmac_slot_e slot)
{
    const transfer_instance_t* handle = drv_transfer_get_handle(slot);

    if (handle == NULL) {
        return FW_FAIL;
    }
    return (R_DMAC_Reconfigure(handle->p_ctrl, handle->p_cfg->p_info)
            == FSP_SUCCESS) ? FW_OK : FW_FAIL;
}

fw_status_t drv_dmac_softwarestart(drv_dmac_slot_e slot, drv_dmac_start_mode_e mode)
{
    const transfer_instance_t* handle = drv_transfer_get_handle(slot);

    if (handle == NULL) {
        return FW_FAIL;
    }
    return (R_DMAC_SoftwareStart(handle->p_ctrl, (transfer_start_mode_t)mode)
            == FSP_SUCCESS) ? FW_OK : FW_FAIL;
}

fw_status_t drv_dmac_init(drv_dmac_slot_e slot, void* destination)
{
    const transfer_instance_t* handle = drv_transfer_get_handle(slot);

    if (handle == NULL) {
        return FW_FAIL;
    }

    /* Wire dest buffer (upper layer's filter buffer) and src ADC register.
     * The ADDR[] index used here is the ADC0 channel paired with this DMAC slot. */
    handle->p_cfg->p_info->p_dest = destination;
    handle->p_cfg->p_info->p_src  = (void*)&R_ADC0->ADDR[slot + 1U];

    return drv_dmac_open(slot);
}

/* ===========================================================================
 * ISR entry points + callback table
 *
 * One ISR per DMAC slot. Each is a one-line dispatch - the upper layer is
 * responsible for keeping its callback body as short as possible.
 * (In dca_adcfilter the callback is literally `fullFlag = true;`.)
 * ========================================================================= */

void isr_adc_dmac0(dmac_callback_args_t* pArgs)
{
    (void)pArgs;
    if (cbDmac[0] != NULL) {
        cbDmac[0]();
    }
}

void isr_adc_dmac1(dmac_callback_args_t* pArgs)
{
    (void)pArgs;
    if (cbDmac[1] != NULL) {
        cbDmac[1]();
    }
}

void isr_adc_dmac2(dmac_callback_args_t* pArgs)
{
    (void)pArgs;
    if (cbDmac[2] != NULL) {
        cbDmac[2]();
    }
}

void isr_adc_dmac3(dmac_callback_args_t* pArgs)
{
    (void)pArgs;
    if (cbDmac[3] != NULL) {
        cbDmac[3]();
    }
}

void drv_dmac_0_register_callback(drv_dmac_callback_t cb)
{
    cbDmac[0] = cb;
}

void drv_dmac_1_register_callback(drv_dmac_callback_t cb)
{
    cbDmac[1] = cb;
}

void drv_dmac_2_register_callback(drv_dmac_callback_t cb)
{
    cbDmac[2] = cb;
}

void drv_dmac_3_register_callback(drv_dmac_callback_t cb)
{
    cbDmac[3] = cb;
}
