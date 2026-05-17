#ifndef DRV_GPIO_H_
#define DRV_GPIO_H_

#include "r_ioport_api.h"
#include "r_ioport.h"

#include "../tools/fw_types.h"

/* GPIO slot enumeration.
 *
 * Note: convention in the original project is "outputs first, then inputs"
 * - this lets a single `<= LAST_OUT` / `>= FIRST_IN` comparison validate
 * direction without a separate table. The pin map below mirrors that order.
 *
 * Physical pin assignments are redacted in this portfolio copy.
 */
typedef enum {
    /* Output GPIOs */
    E_DRV_GPIO_OUT_A,                                   /* pin assignment redacted */
    E_DRV_GPIO_OUT_B,
    E_DRV_GPIO_OUT_C,
    E_DRV_GPIO_OUT_D,
    E_DRV_GPIO_LAST_OUTPUT = E_DRV_GPIO_OUT_D,

    /* Input GPIOs */
    E_DRV_GPIO_IN_POWER_SW,                             /* wake / power switch */
    E_DRV_GPIO_IN_FOOT_SW,
    E_DRV_GPIO_IN_FOOT_SW_CONN,
    E_DRV_GPIO_IN_BATT_CHARGE,
    E_DRV_GPIO_IN_BATT_FULL,
    E_DRV_GPIO_IN_BATT_PG,
    E_DRV_GPIO_IN_ACCESSORY_CONN,
    E_DRV_GPIO_PORT_MAX,
    E_DRV_GPIO_PORT_UNKNOWN = E_DRV_GPIO_PORT_MAX
} drv_gpio_slot_e;

bool        drv_gpio_pinread (drv_gpio_slot_e slot);
fw_status_t drv_gpio_pinwrite(drv_gpio_slot_e slot, io_level_e level);
void        drv_gpio_on      (drv_gpio_slot_e slot);
void        drv_gpio_off     (drv_gpio_slot_e slot);
fw_status_t drv_gpio_open    (void);

#endif /* DRV_GPIO_H_ */
