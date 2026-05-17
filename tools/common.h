#ifndef COMMON_H_
#define COMMON_H_

/*
 * common.h - Aggregator of project-wide helpers
 *
 * Portfolio note: implementations of error()/info()/dump() are intentionally
 * omitted - this header is declaration-only. In the original project these are
 * routed through SEGGER RTT for non-blocking logging.
 */

#include "fw_types.h"

/* --- Logging macros (stubs in this portfolio) ------------------------------ */
#define error(fmt, ...)   ((void)0)
#define info(fmt, ...)    ((void)0)
#define D_PRINT(fmt, ...) ((void)0)
#define dump(buf, len)    ((void)0)

/* --- Color codes for log strings (no-op here) ------------------------------ */
#define D_COLOR_RED   ""
#define D_COLOR_GREEN ""
#define D_COLOR_NONE  ""

/* --- Time helpers (declaration only) --------------------------------------- */
uint32_t timext_start(void);
bool     timext_expired(uint32_t startTick, uint32_t timeoutMs);

/* --- Utility helpers (declaration only) ------------------------------------ */
uint8_t  get_xor_checksum(const uint8_t* buf, size_t len);

#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

/* --- Compile-time switches -------------------------------------------------- */
#define D_USE_DDI_MUTEX       (1)  /* 0 -> mutex ops compile out to no-op    */
#define D_USE_TH_ACQ          (1)  /* 0 -> th_acq thread inert               */
#define D_USE_TH_SAFETY_CTRL  (1)  /* 0 -> th_safety_ctrl thread inert       */
#define D_USE_TH_COMM         (1)  /* 0 -> th_comm thread inert              */

#endif /* COMMON_H_ */
