#include "drv_gpio.h"
#include "../tools/common.h"

/* Physical pin assignments are redacted - in the original project this maps
 * E_DRV_GPIO_* to BSP_IO_PORT_xx_PIN_yy macros from the BSP. */
static const bsp_io_port_pin_t drvGpioMap[E_DRV_GPIO_PORT_MAX] = {
    [E_DRV_GPIO_OUT_A]              = 0U,               /* pin: redacted */
    [E_DRV_GPIO_OUT_B]              = 0U,               /* pin: redacted */
    [E_DRV_GPIO_OUT_C]              = 0U,               /* pin: redacted */
    [E_DRV_GPIO_OUT_D]              = 0U,               /* pin: redacted */
    [E_DRV_GPIO_IN_POWER_SW]        = 0U,               /* pin: redacted */
    [E_DRV_GPIO_IN_FOOT_SW]         = 0U,               /* pin: redacted */
    [E_DRV_GPIO_IN_FOOT_SW_CONN]    = 0U,               /* pin: redacted */
    [E_DRV_GPIO_IN_BATT_CHARGE]     = 0U,               /* pin: redacted */
    [E_DRV_GPIO_IN_BATT_FULL]       = 0U,               /* pin: redacted */
    [E_DRV_GPIO_IN_BATT_PG]         = 0U,               /* pin: redacted */
    [E_DRV_GPIO_IN_ACCESSORY_CONN]  = 0U,               /* pin: redacted */
};

extern const ioport_instance_ctrl_t g_ioport_ctrl;
extern const ioport_cfg_t           g_bsp_pin_cfg;

static bool is_output(drv_gpio_slot_e slot)
{
    return (slot <= E_DRV_GPIO_LAST_OUTPUT);
}

bool drv_gpio_pinread(drv_gpio_slot_e slot)
{
    bsp_io_level_t level = BSP_IO_LEVEL_LOW;

    if ((slot >= E_DRV_GPIO_PORT_MAX) || is_output(slot)) {
        error("Invalid input slot=%d", slot);
        return LOW;
    }
    if (R_IOPORT_PinRead(&g_ioport_ctrl, drvGpioMap[slot], &level) != FSP_SUCCESS) {
        return LOW;
    }
    return (level == BSP_IO_LEVEL_HIGH) ? HIGH : LOW;
}

fw_status_t drv_gpio_pinwrite(drv_gpio_slot_e slot, io_level_e level)
{
    if ((slot >= E_DRV_GPIO_PORT_MAX) || !is_output(slot)) {
        error("Invalid output slot=%d", slot);
        return FW_FAIL;
    }
    return (R_IOPORT_PinWrite(&g_ioport_ctrl, drvGpioMap[slot], (bsp_io_level_t)level)
            == FSP_SUCCESS) ? FW_OK : FW_FAIL;
}

void drv_gpio_on(drv_gpio_slot_e slot)
{
    (void)drv_gpio_pinwrite(slot, HIGH);
}

void drv_gpio_off(drv_gpio_slot_e slot)
{
    (void)drv_gpio_pinwrite(slot, LOW);
}

fw_status_t drv_gpio_open(void)
{
    return (R_IOPORT_Open(&g_ioport_ctrl, &g_bsp_pin_cfg) == FSP_SUCCESS) ? FW_OK : FW_FAIL;
}
