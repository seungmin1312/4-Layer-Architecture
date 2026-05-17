#include "ddi_dmac.h"
#include "../tools/common.h"

/* Thin wrapper around drv_dmac - same rationale as ddi_adc.c. */

fw_status_t ddi_dmac_open(ddi_dmac_slot_e slot)
{
    if (slot >= E_DDI_DMAC_HANDLE_MAX) {
        return FW_FAIL;
    }
    return drv_dmac_open((drv_dmac_slot_e)slot);
}

fw_status_t ddi_dmac_close(ddi_dmac_slot_e slot)
{
    if (slot >= E_DDI_DMAC_HANDLE_MAX) {
        return FW_FAIL;
    }
    return drv_dmac_close((drv_dmac_slot_e)slot);
}

fw_status_t ddi_dmac_enable(ddi_dmac_slot_e slot)
{
    if (slot >= E_DDI_DMAC_HANDLE_MAX) {
        return FW_FAIL;
    }
    return drv_dmac_enable((drv_dmac_slot_e)slot);
}

fw_status_t ddi_dmac_disable(ddi_dmac_slot_e slot)
{
    if (slot >= E_DDI_DMAC_HANDLE_MAX) {
        return FW_FAIL;
    }
    return drv_dmac_disable((drv_dmac_slot_e)slot);
}

fw_status_t ddi_dmac_reconfigure(ddi_dmac_slot_e slot)
{
    if (slot >= E_DDI_DMAC_HANDLE_MAX) {
        return FW_FAIL;
    }
    return drv_dmac_reconfigure((drv_dmac_slot_e)slot);
}

fw_status_t ddi_dmac_softwarestart(ddi_dmac_slot_e slot, ddi_dmac_start_mode_e mode)
{
    if (slot >= E_DDI_DMAC_HANDLE_MAX) {
        return FW_FAIL;
    }
    return drv_dmac_softwarestart((drv_dmac_slot_e)slot, (drv_dmac_start_mode_e)mode);
}

fw_status_t ddi_dmac_init(ddi_dmac_slot_e slot, void* destination)
{
    if (slot >= E_DDI_DMAC_HANDLE_MAX) {
        return FW_FAIL;
    }
    return drv_dmac_init((drv_dmac_slot_e)slot, destination);
}

void ddi_dmac_0_handle(ddi_dmac_callback_t cb)
{
    drv_dmac_0_register_callback(cb);
}

void ddi_dmac_1_handle(ddi_dmac_callback_t cb)
{
    drv_dmac_1_register_callback(cb);
}

void ddi_dmac_2_handle(ddi_dmac_callback_t cb)
{
    drv_dmac_2_register_callback(cb);
}

void ddi_dmac_3_handle(ddi_dmac_callback_t cb)
{
    drv_dmac_3_register_callback(cb);
}
