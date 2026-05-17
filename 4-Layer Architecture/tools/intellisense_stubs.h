/*
 * intellisense_stubs.h
 *
 * IDE-only stubs for vendor types (Renesas FSP, Azure RTOS ThreadX).
 *
 * Purpose: this portfolio code is not buildable - vendor headers are not
 * checked in. VS Code's C/C++ IntelliSense needs these symbols defined to
 * parse files cleanly so type / macro / enum tokens get semantic
 * highlighting instead of falling back to plain "unknown identifier" color.
 *
 * Activation:
 *   This file is force-included by VS Code's C/C++ extension via
 *   `C_Cpp.default.forcedInclude` (see .vscode/settings.json).
 *   The body is guarded by `__INTELLISENSE__` so it is NEVER seen by a
 *   real compiler - it exists purely for the IDE.
 */
#ifndef PORTFOLIO_INTELLISENSE_STUBS_H_
#define PORTFOLIO_INTELLISENSE_STUBS_H_

#ifdef __INTELLISENSE__

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* ---------------------------------------------------------------------------
 * Azure RTOS ThreadX
 * ------------------------------------------------------------------------- */
typedef unsigned int  UINT;
typedef unsigned long ULONG;

typedef struct TX_MUTEX_STRUCT     { int dummy; } TX_MUTEX;
typedef struct TX_SEMAPHORE_STRUCT { int dummy; } TX_SEMAPHORE;

#define TX_SUCCESS                  ((UINT)0)
#define TX_NO_INSTANCE              ((UINT)0x07)
#define TX_NO_WAIT                  ((ULONG)0)
#define TX_WAIT_FOREVER             ((ULONG)0xFFFFFFFFU)
#define TX_TIMER_TICKS_PER_SECOND   (100UL)

UINT tx_mutex_get    (TX_MUTEX*, ULONG);
UINT tx_mutex_put    (TX_MUTEX*);
UINT tx_semaphore_get(TX_SEMAPHORE*, ULONG);
UINT tx_semaphore_put(TX_SEMAPHORE*);

/* ---------------------------------------------------------------------------
 * Renesas FSP - core
 * ------------------------------------------------------------------------- */
typedef enum {
    FSP_SUCCESS            = 0,
    FSP_ERR_INVALID_POINTER,
    FSP_ERR_NOT_OPEN
} fsp_err_t;

typedef enum { FSP_PRIV_CLOCK_PCLKA = 0 } fsp_priv_clock_t;
uint32_t R_FSP_SystemClockHzGet(fsp_priv_clock_t);

/* ---------------------------------------------------------------------------
 * Renesas FSP - UART
 * ------------------------------------------------------------------------- */
typedef enum { UART_EVENT_TX_COMPLETE = 0, UART_EVENT_RX_COMPLETE } uart_event_t;
typedef enum { UART_DIR_RX = 0, UART_DIR_TX } uart_dir_t;

typedef struct uart_callback_args { uart_event_t event; } uart_callback_args_t;
typedef struct uart_instance { void* p_ctrl; void* p_cfg; } uart_instance_t;

fsp_err_t R_SCI_UART_Open (void*, const void*);
fsp_err_t R_SCI_UART_Read (void*, uint8_t*, uint32_t);
fsp_err_t R_SCI_UART_Write(void*, uint8_t*, uint32_t);
fsp_err_t R_SCI_UART_Abort(void*, uart_dir_t);

/* ---------------------------------------------------------------------------
 * Renesas FSP - ADC
 * ------------------------------------------------------------------------- */
typedef enum { ADC_EVENT_SCAN_COMPLETE = 0 } adc_event_t;
typedef struct adc_callback_args { adc_event_t event; }      adc_callback_args_t;
typedef struct adc_channel_cfg   { int dummy; }              adc_channel_cfg_t;
typedef struct adc_instance      { void* p_ctrl; void* p_cfg; } adc_instance_t;

fsp_err_t R_ADC_Open     (void*, const void*);
fsp_err_t R_ADC_Close    (void*);
fsp_err_t R_ADC_ScanCfg  (void*, const adc_channel_cfg_t*);
fsp_err_t R_ADC_ScanStart(void*);
fsp_err_t R_ADC_ScanStop (void*);

typedef struct { volatile uint16_t ADDR[32]; } R_ADC_Type;
#define R_ADC0  ((R_ADC_Type*)0)
#define R_ADC1  ((R_ADC_Type*)0)

/* ---------------------------------------------------------------------------
 * Renesas FSP - DMAC / transfer
 * ------------------------------------------------------------------------- */
typedef enum {
    TRANSFER_START_MODE_SINGLE = 0,
    TRANSFER_START_MODE_REPEAT
} transfer_start_mode_t;

typedef struct transfer_info       { void* p_src; void* p_dest; }       transfer_info_t;
typedef struct dmac_extended_cfg   { transfer_info_t* p_info; }         transfer_cfg_t;
typedef struct transfer_instance   { void* p_ctrl; transfer_cfg_t* p_cfg; } transfer_instance_t;
typedef struct dmac_callback_args  { int dummy; }                       dmac_callback_args_t;

fsp_err_t R_DMAC_Open         (void*, const void*);
fsp_err_t R_DMAC_Close        (void*);
fsp_err_t R_DMAC_Enable       (void*);
fsp_err_t R_DMAC_Disable      (void*);
fsp_err_t R_DMAC_Reconfigure  (void*, const transfer_info_t*);
fsp_err_t R_DMAC_SoftwareStart(void*, transfer_start_mode_t);

/* ---------------------------------------------------------------------------
 * Renesas FSP - GPT / AGT timer
 * ------------------------------------------------------------------------- */
typedef enum { TIMER_EVENT_CYCLE_END = 0 } timer_event_t;
typedef enum { GPT_IO_PIN_GTIOCA = 0 }     gpt_io_pin_t;
typedef enum { TIMER_DIRECTION_UP = 0 }    timer_direction_t;

typedef struct timer_info {
    timer_direction_t dir;
    uint32_t          period_counts;
    uint32_t          clock_frequency;
} timer_info_t;

typedef struct timer_status { uint8_t state; } timer_status_t;
typedef struct timer_cfg    { uint8_t channel; uint32_t source_div; } timer_cfg_t;
typedef struct timer_instance { void* p_ctrl; const timer_cfg_t* p_cfg; } timer_instance_t;
typedef struct timer_callback_args { timer_event_t event; } timer_callback_args_t;

fsp_err_t R_GPT_Open        (void*, const timer_cfg_t*);
fsp_err_t R_GPT_Close       (void*);
fsp_err_t R_GPT_Start       (void*);
fsp_err_t R_GPT_Stop        (void*);
fsp_err_t R_GPT_Reset       (void*);
fsp_err_t R_GPT_InfoGet     (void*, timer_info_t*);
fsp_err_t R_GPT_StatusGet   (void*, timer_status_t*);
fsp_err_t R_GPT_PeriodSet   (void*, uint32_t);
fsp_err_t R_GPT_DutyCycleSet(void*, uint32_t, gpt_io_pin_t);

fsp_err_t R_AGT_Open  (void*, const timer_cfg_t*);
fsp_err_t R_AGT_Close (void*);
fsp_err_t R_AGT_Start (void*);
fsp_err_t R_AGT_Stop  (void*);
fsp_err_t R_AGT_Reset (void*);

typedef struct { volatile uint32_t GTWP; volatile uint32_t GTCCR[8]; } R_GPT_Type;
#define R_GPT1  ((R_GPT_Type*)0)

/* ---------------------------------------------------------------------------
 * Renesas FSP - IOPORT
 * ------------------------------------------------------------------------- */
typedef uint32_t bsp_io_port_pin_t;
typedef enum { BSP_IO_LEVEL_LOW = 0, BSP_IO_LEVEL_HIGH } bsp_io_level_t;
typedef struct ioport_instance_ctrl { int dummy; } ioport_instance_ctrl_t;
typedef struct ioport_cfg          { int dummy; } ioport_cfg_t;

fsp_err_t R_IOPORT_Open    (const ioport_instance_ctrl_t*, const ioport_cfg_t*);
fsp_err_t R_IOPORT_PinRead (const ioport_instance_ctrl_t*, bsp_io_port_pin_t, bsp_io_level_t*);
fsp_err_t R_IOPORT_PinWrite(const ioport_instance_ctrl_t*, bsp_io_port_pin_t, bsp_io_level_t);

#endif /* __INTELLISENSE__ */
#endif /* PORTFOLIO_INTELLISENSE_STUBS_H_ */
