#ifndef DRV_UART_H_
#define DRV_UART_H_

#include "r_uart_api.h"             /* Renesas FSP UART API (vendor) */

#include "../tools/fw_types.h"

typedef void (*drv_uart_callback_t)(void);

/* UART instance slots - meaning is assigned by upper layers.
 * drv/ only knows that slot N maps to FSP handle uartHandle[N]. */
typedef enum {
    E_DRV_UART_CONSOLE,                                 /* &g_uart0 */
    E_DRV_UART_LCD,                                     /* &g_uart4 */
    E_DRV_UART_HANDLE_MAX,
    E_DRV_UART_HANDLE_UNKNOWN = E_DRV_UART_HANDLE_MAX
} drv_uart_slot_e;

fw_status_t drv_uart_open(drv_uart_slot_e slot);
size_t      drv_uart_read(drv_uart_slot_e slot, uint8_t* buf, size_t size);
size_t      drv_uart_write(drv_uart_slot_e slot, const uint8_t* data, size_t size);
fw_status_t drv_uart_abort_rx(drv_uart_slot_e slot);

/* Per-instance callback registration - upper layer (ddi_uart) injects its
 * handler here at init time. drv_uart never includes ddi_uart.h. */
void drv_uart_console_rx_register_callback(drv_uart_callback_t cb);
void drv_uart_console_tx_register_callback(drv_uart_callback_t cb);
void drv_uart_lcd_rx_register_callback(drv_uart_callback_t cb);
void drv_uart_lcd_tx_register_callback(drv_uart_callback_t cb);

#endif /* DRV_UART_H_ */
