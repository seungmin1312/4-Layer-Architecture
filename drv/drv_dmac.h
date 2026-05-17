#ifndef DRV_DMAC_H_
#define DRV_DMAC_H_

#include "r_transfer_api.h"
#include "r_dmac.h"

#include "../tools/fw_types.h"

typedef void (*drv_dmac_callback_t)(void);

/* Four ADC0 channels driven by DMAC.
 * Each slot independently fires its ISR when its block fills. */
typedef enum {
    E_DRV_DMAC_0,                                       /* ADC0 ch1 */
    E_DRV_DMAC_1,                                       /* ADC0 ch2 */
    E_DRV_DMAC_2,                                       /* ADC0 ch3 */
    E_DRV_DMAC_3,                                       /* ADC0 ch4 */
    E_DRV_DMAC_HANDLE_MAX,
    E_DRV_DMAC_HANDLE_UNKNOWN = E_DRV_DMAC_HANDLE_MAX
} drv_dmac_slot_e;

typedef enum {
    E_DRV_DMAC_MODE_SINGLE = 0,
    E_DRV_DMAC_MODE_REPEAT = 1,
    E_DRV_DMAC_MODE_MAX,
    E_DRV_DMAC_MODE_UNKNOWN = E_DRV_DMAC_MODE_MAX
} drv_dmac_start_mode_e;

fw_status_t drv_dmac_open          (drv_dmac_slot_e slot);
fw_status_t drv_dmac_close         (drv_dmac_slot_e slot);
fw_status_t drv_dmac_enable        (drv_dmac_slot_e slot);
fw_status_t drv_dmac_disable       (drv_dmac_slot_e slot);
fw_status_t drv_dmac_reconfigure   (drv_dmac_slot_e slot);
fw_status_t drv_dmac_softwarestart (drv_dmac_slot_e slot, drv_dmac_start_mode_e mode);
fw_status_t drv_dmac_init          (drv_dmac_slot_e slot, void* destination);

void drv_dmac_0_register_callback(drv_dmac_callback_t cb);
void drv_dmac_1_register_callback(drv_dmac_callback_t cb);
void drv_dmac_2_register_callback(drv_dmac_callback_t cb);
void drv_dmac_3_register_callback(drv_dmac_callback_t cb);

#endif /* DRV_DMAC_H_ */
