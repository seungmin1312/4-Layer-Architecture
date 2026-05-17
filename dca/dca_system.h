#ifndef DCA_SYSTEM_H_
#define DCA_SYSTEM_H_

#include "../tools/common.h"
#include "../ddi/ddi_gpio.h"

/* System-wide GPIO-derived state. Always read via snapshot from other threads. */
typedef struct {
    onoff_e    powerSW;
    onoff_e    footSWTrig;
    connect_e  footSWConn;
    connect_e  accessoryConn;
    charge_e   batteryCharge;
    onoff_e    batteryFull;
    onoff_e    batteryPwrGood;
} dca_system_info_t;

/* Push-model change notification.
 * The mask packs which fields changed in this update, so the listener can
 * react selectively without diffing two snapshots. */
typedef enum {
    E_DCA_SYSTEM_CHANGED_NONE          = 0U,
    E_DCA_SYSTEM_CHANGED_POWER_SW      = (1U << 0),
    E_DCA_SYSTEM_CHANGED_FOOT_SW_TRIG  = (1U << 1),
    E_DCA_SYSTEM_CHANGED_FOOT_SW_CONN  = (1U << 2),
    E_DCA_SYSTEM_CHANGED_ACCESSORY     = (1U << 3),
    E_DCA_SYSTEM_CHANGED_BATT_CHARGE   = (1U << 4),
    E_DCA_SYSTEM_CHANGED_BATT_FULL     = (1U << 5),
    E_DCA_SYSTEM_CHANGED_BATT_PWRGOOD  = (1U << 6),
    E_DCA_SYSTEM_CHANGED_MAX,
    E_DCA_SYSTEM_CHANGED_UNKNOWN       = E_DCA_SYSTEM_CHANGED_MAX
} dca_system_changed_mask_e;

typedef void (*dca_system_changed_callback_t)(uint32_t changedMask,
                                              const dca_system_info_t* sysInfo);

/* Single-thread, lock-free direct pointer - same-module use only. */
dca_system_info_t* dca_system_get_info(void);

/* Cross-thread safe snapshot copy. */
fw_status_t dca_system_get_snapshot(dca_system_info_t* out);

void dca_system_set_changed_callback(dca_system_changed_callback_t cb);
void dca_system_init(void);
void dca_system_poll_all_inputs(void);

#endif /* DCA_SYSTEM_H_ */
