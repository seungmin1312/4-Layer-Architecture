#ifndef FW_TYPES_H_
#define FW_TYPES_H_

/*
 * fw_types.h - Project-wide minimal types
 *
 * Intentionally minimal: keep this header free of cross-layer includes so it
 * never participates in circular dependencies. Both drv/ and ddi/ headers
 * include only this file (never common.h).
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

typedef enum {
    OFF = 0,
    ON,
    ONOFF_MAX,
    ONOFF_UNKNOWN = ONOFF_MAX
} onoff_e;

typedef enum {
    DISCONN = 0,
    CONN,
    CONNECT_MAX,
    CONNECT_UNKNOWN = CONNECT_MAX
} connect_e;

typedef enum {
    DISCHARGE = 0,
    CHARGE,
    CHARGE_MAX,
    CHARGE_UNKNOWN = CHARGE_MAX
} charge_e;

typedef enum {
    LOW = 0,
    HIGH,
    IO_LEVEL_MAX,
    IO_LEVEL_UNKNOWN = IO_LEVEL_MAX
} io_level_e;

/* Unified return type used across all layers.
 * Detailed failure cause is logged via error() inside each function.
 */
typedef enum {
    FW_OK   = 0,
    FW_FAIL = 1,
    FW_STATUS_MAX,
    FW_STATUS_UNKNOWN = FW_STATUS_MAX
} fw_status_t;

#endif /* FW_TYPES_H_ */
