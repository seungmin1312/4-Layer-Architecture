#ifndef DDI_ADC_H_
#define DDI_ADC_H_

#include "../tools/fw_types.h"
#include "../drv/drv_adc.h"

typedef void (*ddi_adc_callback_t)(void);

/* Mirrors drv_adc_slot_e but lives in its own namespace so DCA never refers
 * to drv_*. This is the "abstraction tax" - one extra enum, one extra cast
 * inside ddi - in exchange for upper layers never having to know about FSP. */
typedef enum {
    E_DDI_ADC_0,
    E_DDI_ADC_1,
    E_DDI_ADC_HANDLE_MAX,
    E_DDI_ADC_HANDLE_UNKNOWN = E_DDI_ADC_HANDLE_MAX
} ddi_adc_slot_e;

typedef enum {
    E_DDI_ADC_1_CHANNEL_0,
    E_DDI_ADC_1_CHANNEL_18,
    E_DDI_ADC_1_CHANNEL_19,
    E_DDI_ADC_1_CHANNEL_20,
    E_DDI_ADC_1_CHANNEL_21,
    E_DDI_ADC_1_CHANNEL_22,
    E_DDI_ADC_CHANNEL_MAX,
    E_DDI_ADC_CHANNEL_UNKNOWN = E_DDI_ADC_CHANNEL_MAX
} ddi_adc_channel_slot_e;

fw_status_t ddi_adc_open      (ddi_adc_slot_e slot);
fw_status_t ddi_adc_close     (ddi_adc_slot_e slot);
fw_status_t ddi_adc_scan_cfg  (ddi_adc_slot_e slot);
fw_status_t ddi_adc_scan_start(ddi_adc_slot_e slot);
fw_status_t ddi_adc_scan_stop (ddi_adc_slot_e slot);
fw_status_t ddi_adc_get_register_data(ddi_adc_channel_slot_e slot, uint16_t* out);

fw_status_t ddi_adc_init(ddi_adc_slot_e slot);

/* ADC0 scan-complete handler registration (forwarded to drv_adc). */
void ddi_adc_0_handle(ddi_adc_callback_t cb);

#endif /* DDI_ADC_H_ */
