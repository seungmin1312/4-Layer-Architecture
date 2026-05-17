#ifndef DRV_ADC_H_
#define DRV_ADC_H_

#include "r_adc_api.h"
#include "r_adc.h"

#include "../tools/fw_types.h"

typedef void (*drv_adc_callback_t)(void);

typedef enum {
    E_DRV_ADC_0,                                        /* RF feedback (DMAC-backed) */
    E_DRV_ADC_1,                                        /* Slow sensors (ELC-triggered) */
    E_DRV_ADC_HANDLE_MAX,
    E_DRV_ADC_HANDLE_UNKNOWN = E_DRV_ADC_HANDLE_MAX
} drv_adc_slot_e;

/* ADC1 channel-level slots used for register-direct reads. */
typedef enum {
    E_DRV_ADC_1_CHANNEL_0,
    E_DRV_ADC_1_CHANNEL_18,
    E_DRV_ADC_1_CHANNEL_19,
    E_DRV_ADC_1_CHANNEL_20,
    E_DRV_ADC_1_CHANNEL_21,
    E_DRV_ADC_1_CHANNEL_22,
    E_DRV_ADC_CHANNEL_MAX,
    E_DRV_ADC_CHANNEL_UNKNOWN = E_DRV_ADC_CHANNEL_MAX
} drv_adc_channel_slot_e;

typedef struct {
    const adc_instance_t*    instance;
    const adc_channel_cfg_t* channelCfg;
} drv_adc_handle_t;

fw_status_t drv_adc_open      (drv_adc_slot_e slot);
fw_status_t drv_adc_close     (drv_adc_slot_e slot);
fw_status_t drv_adc_scan_cfg  (drv_adc_slot_e slot);
fw_status_t drv_adc_scan_start(drv_adc_slot_e slot);
fw_status_t drv_adc_scan_stop (drv_adc_slot_e slot);
fw_status_t drv_adc_get_register_data(drv_adc_channel_slot_e slot, uint16_t* out);

fw_status_t drv_adc_init(drv_adc_slot_e slot);

/* ADC0 scan-complete ISR hook - registered by the upper layer (ddi_adc). */
void drv_adc_0_register_callback(drv_adc_callback_t cb);

#endif /* DRV_ADC_H_ */
