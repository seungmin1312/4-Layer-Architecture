#include "dca_system.h"
#include "../ddi/ddi_mutex.h"

/* ===========================================================================
 * dca_system - GPIO event aggregation + snapshot-protected shared state
 *
 * Three patterns on display:
 *
 *   1. **Upward callback registration** - this module never #includes any
 *      app/ header. It exposes a `set_changed_callback()` and lets the
 *      listener (typically app_monitor) hand in a function pointer at init.
 *
 *   2. **Snapshot copy under mutex** - shared state `systemInfo` lives in
 *      this module. Other tasks read it via `dca_system_get_snapshot()`
 *      which holds the mutex only for the duration of a `memcpy`.
 *
 *   3. **Per-pin debounced event -> domain meaning** - DDI raises a
 *      generic PRESSED/RELEASED edge; THIS layer is the first to know
 *      what that edge means (powerSW on vs off, foot switch trigger,
 *      battery charge state, etc.). Crucially, the DRV/DDI layers never
 *      know these semantic names exist.
 * ========================================================================= */

static dca_system_info_t              systemInfo;
static dca_system_changed_callback_t  dcaSystemChange = NULL;

/* Compile-time switchable mutex - same pattern as dca_adcfilter. */
static inline void lock_system_info(void)
{
#if D_USE_DDI_MUTEX
    (void)ddi_mutex_get(E_DDI_MUTEX_SYSTEM_INFO, D_DDI_MUTEX_WAIT_FOREVER);
#endif
}

static inline void unlock_system_info(void)
{
#if D_USE_DDI_MUTEX
    (void)ddi_mutex_put(E_DDI_MUTEX_SYSTEM_INFO);
#endif
}

dca_system_info_t* dca_system_get_info(void)
{
    return &systemInfo;
}

fw_status_t dca_system_get_snapshot(dca_system_info_t* out)
{
    if (out == NULL) {
        return FW_FAIL;
    }
    lock_system_info();
    memcpy(out, &systemInfo, sizeof(*out));
    unlock_system_info();
    return FW_OK;
}

void dca_system_set_changed_callback(dca_system_changed_callback_t cb)
{
    dcaSystemChange = cb;
}

static void notify_changed(uint32_t changedMask)
{
    if ((dcaSystemChange != NULL) && (changedMask != E_DCA_SYSTEM_CHANGED_NONE)) {
        dcaSystemChange(changedMask, &systemInfo);
    }
}

/* ===========================================================================
 * Per-pin event handlers. Registered to DDI in `register_gpio_event_handles`.
 *
 * Each takes the generic PRESSED/RELEASED edge from DDI debounce and maps it
 * to a semantic field. If the field actually changed, we set the matching
 * bit in the changed mask and forward to the registered upper-layer callback.
 * ========================================================================= */

static void on_power_sw(ddi_gpio_input_event_e event)
{
    onoff_e prev = systemInfo.powerSW;

    systemInfo.powerSW = (event == E_DDI_GPIO_INPUT_EVENT_PRESSED) ? ON : OFF;
    if (prev != systemInfo.powerSW) {
        notify_changed(E_DCA_SYSTEM_CHANGED_POWER_SW);
    }
}

static void on_foot_sw(ddi_gpio_input_event_e event)
{
    onoff_e prev = systemInfo.footSWTrig;

    systemInfo.footSWTrig = (event == E_DDI_GPIO_INPUT_EVENT_PRESSED) ? ON : OFF;
    if (prev != systemInfo.footSWTrig) {
        notify_changed(E_DCA_SYSTEM_CHANGED_FOOT_SW_TRIG);
    }
}

static void on_foot_sw_conn(ddi_gpio_input_event_e event)
{
    connect_e prev = systemInfo.footSWConn;

    systemInfo.footSWConn = (event == E_DDI_GPIO_INPUT_EVENT_PRESSED) ? CONN : DISCONN;
    if (prev != systemInfo.footSWConn) {
        notify_changed(E_DCA_SYSTEM_CHANGED_FOOT_SW_CONN);
    }
}

static void on_batt_charge(ddi_gpio_input_event_e event)
{
    charge_e prev = systemInfo.batteryCharge;

    systemInfo.batteryCharge = (event == E_DDI_GPIO_INPUT_EVENT_PRESSED) ? CHARGE : DISCHARGE;
    if (prev != systemInfo.batteryCharge) {
        notify_changed(E_DCA_SYSTEM_CHANGED_BATT_CHARGE);
    }
}

static void on_batt_full(ddi_gpio_input_event_e event)
{
    onoff_e prev = systemInfo.batteryFull;

    systemInfo.batteryFull = (event == E_DDI_GPIO_INPUT_EVENT_PRESSED) ? ON : OFF;
    if (prev != systemInfo.batteryFull) {
        notify_changed(E_DCA_SYSTEM_CHANGED_BATT_FULL);
    }
}

static void on_batt_pwrgood(ddi_gpio_input_event_e event)
{
    onoff_e prev = systemInfo.batteryPwrGood;

    systemInfo.batteryPwrGood = (event == E_DDI_GPIO_INPUT_EVENT_PRESSED) ? ON : OFF;
    if (prev != systemInfo.batteryPwrGood) {
        notify_changed(E_DCA_SYSTEM_CHANGED_BATT_PWRGOOD);
    }
}

static void on_accessory(ddi_gpio_input_event_e event)
{
    connect_e prev = systemInfo.accessoryConn;

    systemInfo.accessoryConn = (event == E_DDI_GPIO_INPUT_EVENT_PRESSED) ? CONN : DISCONN;
    if (prev != systemInfo.accessoryConn) {
        notify_changed(E_DCA_SYSTEM_CHANGED_ACCESSORY);
    }
}

/* Single point that connects every DDI input slot to its DCA handler.
 * Adding a new input is two lines: an `on_xxx` handler above + one row here. */
static void register_gpio_event_handles(void)
{
    ddi_gpio_set_event_handle(E_DDI_GPIO_IN_POWER_SW,        on_power_sw);
    ddi_gpio_set_event_handle(E_DDI_GPIO_IN_FOOT_SW,         on_foot_sw);
    ddi_gpio_set_event_handle(E_DDI_GPIO_IN_FOOT_SW_CONN,    on_foot_sw_conn);
    ddi_gpio_set_event_handle(E_DDI_GPIO_IN_BATT_CHARGE,     on_batt_charge);
    ddi_gpio_set_event_handle(E_DDI_GPIO_IN_BATT_FULL,       on_batt_full);
    ddi_gpio_set_event_handle(E_DDI_GPIO_IN_BATT_PG,         on_batt_pwrgood);
    ddi_gpio_set_event_handle(E_DDI_GPIO_IN_ACCESSORY_CONN,  on_accessory);
}

/* Atomic multi-field init from current pin levels. Single lock window. */
static void init_state_from_pins(dca_system_info_t* info)
{
    lock_system_info();
    info->powerSW        = ddi_gpio_is_active(E_DDI_GPIO_IN_POWER_SW)        ? ON     : OFF;
    info->footSWTrig     = ddi_gpio_is_active(E_DDI_GPIO_IN_FOOT_SW)         ? ON     : OFF;
    info->footSWConn     = ddi_gpio_is_active(E_DDI_GPIO_IN_FOOT_SW_CONN)    ? CONN   : DISCONN;
    info->accessoryConn  = ddi_gpio_is_active(E_DDI_GPIO_IN_ACCESSORY_CONN)  ? CONN   : DISCONN;
    info->batteryCharge  = ddi_gpio_is_active(E_DDI_GPIO_IN_BATT_CHARGE)     ? CHARGE : DISCHARGE;
    info->batteryFull    = ddi_gpio_is_active(E_DDI_GPIO_IN_BATT_FULL)       ? ON     : OFF;
    info->batteryPwrGood = ddi_gpio_is_active(E_DDI_GPIO_IN_BATT_PG)         ? ON     : OFF;
    unlock_system_info();
}

void dca_system_init(void)
{
    if (ddi_gpio_open() != FW_OK) {
        error("GPIO open fail");
        return;
    }

    ddi_gpio_init_default_output();
    ddi_gpio_init_default_input();

    init_state_from_pins(&systemInfo);
    register_gpio_event_handles();
}

/* Called from th_safety_ctrl @1ms. Each edge fires the corresponding
 * `on_*` callback above; the upper-layer changed-callback is fanned out
 * from there. dca_system never reaches up via #include. */
void dca_system_poll_all_inputs(void)
{
    ddi_gpio_poll_all_inputs();
}
