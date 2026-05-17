#ifndef DDI_DMAC_H_
#define DDI_DMAC_H_

#include "../tools/fw_types.h"
#include "../drv/drv_dmac.h"

typedef void (*ddi_dmac_callback_t)(void);

typedef enum {
    E_DDI_DMAC_0,
    E_DDI_DMAC_1,
    E_DDI_DMAC_2,
    E_DDI_DMAC_3,
    E_DDI_DMAC_HANDLE_MAX,
    E_DDI_DMAC_HANDLE_UNKNOWN = E_DDI_DMAC_HANDLE_MAX
} ddi_dmac_slot_e;

typedef enum {
    E_DDI_DMAC_MODE_SINGLE = 0,
    E_DDI_DMAC_MODE_REPEAT = 1,
    E_DDI_DMAC_MODE_MAX,
    E_DDI_DMAC_MODE_UNKNOWN = E_DDI_DMAC_MODE_MAX
} ddi_dmac_start_mode_e;

fw_status_t ddi_dmac_open         (ddi_dmac_slot_e slot);
fw_status_t ddi_dmac_close        (ddi_dmac_slot_e slot);
fw_status_t ddi_dmac_enable       (ddi_dmac_slot_e slot);
fw_status_t ddi_dmac_disable      (ddi_dmac_slot_e slot);
fw_status_t ddi_dmac_reconfigure  (ddi_dmac_slot_e slot);
fw_status_t ddi_dmac_softwarestart(ddi_dmac_slot_e slot, ddi_dmac_start_mode_e mode);
fw_status_t ddi_dmac_init         (ddi_dmac_slot_e slot, void* destination);

void ddi_dmac_0_handle(ddi_dmac_callback_t cb);
void ddi_dmac_1_handle(ddi_dmac_callback_t cb);
void ddi_dmac_2_handle(ddi_dmac_callback_t cb);
void ddi_dmac_3_handle(ddi_dmac_callback_t cb);

#endif /* DDI_DMAC_H_ */
