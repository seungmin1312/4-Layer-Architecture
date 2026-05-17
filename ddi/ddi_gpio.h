#ifndef DDI_GPIO_H_
#define DDI_GPIO_H_

#include "../tools/fw_types.h"
#include "../drv/drv_gpio.h"

#define D_INPUT_GPIO_DEBOUNCE_CNT  (3U)

typedef enum {
    /* Output GPIOs */
    E_DDI_GPIO_OUT_A,
    E_DDI_GPIO_OUT_B,
    E_DDI_GPIO_OUT_C,
    E_DDI_GPIO_OUT_D,
    E_DDI_GPIO_LAST_OUTPUT = E_DDI_GPIO_OUT_D,

    /* Input GPIOs */
    E_DDI_GPIO_IN_POWER_SW,
    E_DDI_GPIO_IN_FOOT_SW,
    E_DDI_GPIO_IN_FOOT_SW_CONN,
    E_DDI_GPIO_IN_BATT_CHARGE,
    E_DDI_GPIO_IN_BATT_FULL,
    E_DDI_GPIO_IN_BATT_PG,
    E_DDI_GPIO_IN_ACCESSORY_CONN,
    E_DDI_GPIO_PORT_MAX,
    E_DDI_GPIO_PORT_UNKNOWN = E_DDI_GPIO_PORT_MAX
} ddi_gpio_slot_e;

typedef enum {
    E_DDI_GPIO_INPUT_EVENT_PRESSED,
    E_DDI_GPIO_INPUT_EVENT_RELEASED,
    E_DDI_GPIO_INPUT_EVENT_NONE,
    E_DDI_GPIO_INPUT_EVENT_MAX,
    E_DDI_GPIO_INPUT_EVENT_UNKNOWN = E_DDI_GPIO_INPUT_EVENT_MAX
} ddi_gpio_input_event_e;

/* Forward-declared debounce state - implementation kept internal. */
typedef struct debounce_s debounce_t;

/* Per-slot static config: debounce handle, active polarity, name for logs. */
typedef struct {
    debounce_t* debounceHandle;
    io_level_e  activeType;
    const char* name;
} ddi_gpio_config_t;

typedef void (*ddi_gpio_event_callback_t)(ddi_gpio_input_event_e event);

bool        ddi_gpio_pinread (ddi_gpio_slot_e slot);
void        ddi_gpio_on      (ddi_gpio_slot_e slot);
void        ddi_gpio_off     (ddi_gpio_slot_e slot);
void        ddi_gpio_activate(ddi_gpio_slot_e slot);
void        ddi_gpio_deactivate(ddi_gpio_slot_e slot);
fw_status_t ddi_gpio_open    (void);
bool        ddi_gpio_is_active(ddi_gpio_slot_e slot);

void ddi_gpio_init_default_output(void);
void ddi_gpio_init_default_input (void);

/* Per-slot upward-callback registration.
 * dca_system installs handlers here at init time. */
void ddi_gpio_set_event_handle(ddi_gpio_slot_e slot, ddi_gpio_event_callback_t cb);

/* Polls every input slot. Debounced edges trigger the registered callback.
 * Called from the safety_ctrl thread at 1ms cadence. */
void ddi_gpio_poll_all_inputs(void);

#endif /* DDI_GPIO_H_ */
