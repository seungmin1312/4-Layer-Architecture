#include "ddi_gpio.h"
#include "../tools/common.h"

/* ===========================================================================
 * ddi_gpio - data-driven GPIO with debounce + upward callbacks
 *
 * Two design ideas demonstrated here:
 *
 *  1. **Config table over switch-case.** All per-pin state (active polarity,
 *     debounce handle, log name) lives in a single static table indexed by
 *     slot enum. Adding a new input pin is one row in the table.
 *
 *  2. **Upward dispatch via callback table.** Each input slot has a
 *     function-pointer slot in ddi_gpio_event[]. The DCA layer installs its
 *     handlers via ddi_gpio_set_event_handle() at init time. From then on,
 *     ddi_gpio never reaches "up" via #include - it only calls through the
 *     function pointer.
 *
 * Debounce implementation itself is a small state machine kept private;
 * concrete debounce_t definition is intentionally omitted here.
 * ========================================================================= */

typedef enum {
    DEBOUNCE_LOW = 0,
    DEBOUNCE_HIGH,
    DEBOUNCE_LEVEL_MAX,
    DEBOUNCE_LEVEL_UNKNOWN = DEBOUNCE_LEVEL_MAX
} debounce_level_e;

typedef enum {
    DEBOUNCE_EVT_NONE = 0,
    DEBOUNCE_EVT_RISE,
    DEBOUNCE_EVT_FALL,
    DEBOUNCE_EVT_MAX,
    DEBOUNCE_EVT_UNKNOWN = DEBOUNCE_EVT_MAX
} debounce_event_e;

/* Concrete debounce state - kept private to this file. */
struct debounce_s {
    debounce_level_e current;
    debounce_level_e pending;
    uint8_t          counter;
    uint8_t          detectCount;
};

static void debounce_init(debounce_t* deb, debounce_level_e initLevel, uint8_t detectCount)
{
    deb->current     = initLevel;
    deb->pending     = initLevel;
    deb->counter     = 0U;
    deb->detectCount = detectCount;
}

static debounce_event_e debounce_update(debounce_t* deb, debounce_level_e sample)
{
    debounce_event_e evt = DEBOUNCE_EVT_NONE;

    if (sample == deb->current) {
        deb->counter = 0U;
        deb->pending = deb->current;
        return DEBOUNCE_EVT_NONE;
    }
    if (sample != deb->pending) {
        deb->pending = sample;
        deb->counter = 1U;
        return DEBOUNCE_EVT_NONE;
    }
    deb->counter++;
    if (deb->counter < deb->detectCount) {
        return DEBOUNCE_EVT_NONE;
    }

    evt = (sample == DEBOUNCE_HIGH) ? DEBOUNCE_EVT_RISE : DEBOUNCE_EVT_FALL;
    deb->current = sample;
    deb->counter = 0U;
    return evt;
}

/* One static debounce_t per input slot. */
static debounce_t debPowerSw;
static debounce_t debFootSw;
static debounce_t debFootSwConn;
static debounce_t debBattCharge;
static debounce_t debBattFull;
static debounce_t debBattPg;
static debounce_t debAccessoryConn;

/* Callback table - one entry per slot, indexed by ddi_gpio_slot_e. */
static ddi_gpio_event_callback_t ddiGpioEvent[E_DDI_GPIO_PORT_MAX] = { NULL };

/* Single source of truth for per-pin config. */
static const ddi_gpio_config_t gpioConfigTable[E_DDI_GPIO_PORT_MAX] = {
    [E_DDI_GPIO_OUT_A]              = { NULL,                HIGH, "OUT_A"          },
    [E_DDI_GPIO_OUT_B]              = { NULL,                HIGH, "OUT_B"          },
    [E_DDI_GPIO_OUT_C]              = { NULL,                HIGH, "OUT_C"          },
    [E_DDI_GPIO_OUT_D]              = { NULL,                LOW,  "OUT_D"          },

    [E_DDI_GPIO_IN_POWER_SW]        = { &debPowerSw,         HIGH, "POWER_SW"       },
    [E_DDI_GPIO_IN_FOOT_SW]         = { &debFootSw,          LOW,  "FOOT_SW"        },
    [E_DDI_GPIO_IN_FOOT_SW_CONN]    = { &debFootSwConn,      LOW,  "FOOT_SW_CONN"   },
    [E_DDI_GPIO_IN_BATT_CHARGE]     = { &debBattCharge,      LOW,  "BATT_CHARGE"    },
    [E_DDI_GPIO_IN_BATT_FULL]       = { &debBattFull,        HIGH, "BATT_FULL"      },
    [E_DDI_GPIO_IN_BATT_PG]         = { &debBattPg,          LOW,  "BATT_PG"        },
    [E_DDI_GPIO_IN_ACCESSORY_CONN]  = { &debAccessoryConn,   LOW,  "ACCESSORY_CONN" },
};

static debounce_level_e raw_to_logical(ddi_gpio_slot_e slot, bool raw)
{
    /* When active-HIGH, raw HIGH means logical HIGH (active).
     * When active-LOW,  raw LOW  means logical HIGH (active). */
    if (gpioConfigTable[slot].activeType == HIGH) {
        return raw ? DEBOUNCE_HIGH : DEBOUNCE_LOW;
    }
    return raw ? DEBOUNCE_LOW : DEBOUNCE_HIGH;
}

bool ddi_gpio_pinread(ddi_gpio_slot_e slot)
{
    return drv_gpio_pinread((drv_gpio_slot_e)slot);
}

void ddi_gpio_on(ddi_gpio_slot_e slot)
{
    drv_gpio_on((drv_gpio_slot_e)slot);
}

void ddi_gpio_off(ddi_gpio_slot_e slot)
{
    drv_gpio_off((drv_gpio_slot_e)slot);
}

void ddi_gpio_activate(ddi_gpio_slot_e slot)
{
    if (gpioConfigTable[slot].activeType == HIGH) {
        ddi_gpio_on(slot);
    } else {
        ddi_gpio_off(slot);
    }
}

void ddi_gpio_deactivate(ddi_gpio_slot_e slot)
{
    if (gpioConfigTable[slot].activeType == HIGH) {
        ddi_gpio_off(slot);
    } else {
        ddi_gpio_on(slot);
    }
}

fw_status_t ddi_gpio_open(void)
{
    return drv_gpio_open();
}

bool ddi_gpio_is_active(ddi_gpio_slot_e slot)
{
    return (raw_to_logical(slot, ddi_gpio_pinread(slot)) == DEBOUNCE_HIGH);
}

void ddi_gpio_init_default_output(void)
{
    int slot = 0;

    for (slot = 0; slot <= (int)E_DDI_GPIO_LAST_OUTPUT; slot++) {
        ddi_gpio_deactivate((ddi_gpio_slot_e)slot);
    }
}

void ddi_gpio_init_default_input(void)
{
    ddi_gpio_slot_e          slot     = E_DDI_GPIO_IN_POWER_SW;
    const ddi_gpio_config_t* cfg      = NULL;
    debounce_level_e         initLvl  = DEBOUNCE_LOW;

    for (slot = E_DDI_GPIO_IN_POWER_SW; slot < E_DDI_GPIO_PORT_MAX; slot++) {
        cfg = &gpioConfigTable[slot];
        if (cfg->debounceHandle != NULL) {
            /* Initialize to "inactive" polarity. */
            initLvl = (cfg->activeType == HIGH) ? DEBOUNCE_LOW : DEBOUNCE_HIGH;
            debounce_init(cfg->debounceHandle, initLvl, D_INPUT_GPIO_DEBOUNCE_CNT);
        }
    }
}

static ddi_gpio_input_event_e poll_one(ddi_gpio_slot_e slot)
{
    debounce_t*       deb    = gpioConfigTable[slot].debounceHandle;
    debounce_level_e  sample = DEBOUNCE_LOW;
    debounce_event_e  event  = DEBOUNCE_EVT_NONE;

    if (deb == NULL) {
        return E_DDI_GPIO_INPUT_EVENT_NONE;
    }
    sample = raw_to_logical(slot, ddi_gpio_pinread(slot));
    event  = debounce_update(deb, sample);

    if (event == DEBOUNCE_EVT_RISE) {
        return E_DDI_GPIO_INPUT_EVENT_PRESSED;
    }
    if (event == DEBOUNCE_EVT_FALL) {
        return E_DDI_GPIO_INPUT_EVENT_RELEASED;
    }
    return E_DDI_GPIO_INPUT_EVENT_NONE;
}

void ddi_gpio_set_event_handle(ddi_gpio_slot_e slot, ddi_gpio_event_callback_t cb)
{
    if (slot < E_DDI_GPIO_PORT_MAX) {
        ddiGpioEvent[slot] = cb;
    }
}

void ddi_gpio_poll_all_inputs(void)
{
    ddi_gpio_slot_e        slot  = E_DDI_GPIO_IN_POWER_SW;
    ddi_gpio_input_event_e event = E_DDI_GPIO_INPUT_EVENT_NONE;

    for (slot = E_DDI_GPIO_IN_POWER_SW; slot < E_DDI_GPIO_PORT_MAX; slot++) {
        event = poll_one(slot);
        if ((event != E_DDI_GPIO_INPUT_EVENT_NONE) && (ddiGpioEvent[slot] != NULL)) {
            ddiGpioEvent[slot](event);
        }
    }
}
