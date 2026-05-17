#include "drv_uart.h"
#include "../tools/common.h"

/* Renesas FSP-generated handles (extern from hal_data.c). */
extern const uart_instance_t g_uart0;                   /* Console */
extern const uart_instance_t g_uart4;                   /* LCD     */

/* Slot enum -> FSP handle table.
 * Adding a new UART slot is a 1-line change here + 1-line in drv_uart.h. */
static const uart_instance_t* uartHandle[E_DRV_UART_HANDLE_MAX] = {
    &g_uart0,
    &g_uart4
};

/* Per-instance callback table - populated by drv_uart_*_register_callback(). */
static drv_uart_callback_t cbConsoleTx = NULL;
static drv_uart_callback_t cbConsoleRx = NULL;
static drv_uart_callback_t cbLcdTx     = NULL;
static drv_uart_callback_t cbLcdRx     = NULL;

static const uart_instance_t* drv_uart_get_handle(drv_uart_slot_e slot)
{
    if (slot >= E_DRV_UART_HANDLE_MAX) {
        error("Invalid UART slot=%d", slot);
        return NULL;
    }
    return uartHandle[slot];
}

fw_status_t drv_uart_open(drv_uart_slot_e slot)
{
    const uart_instance_t* handle = drv_uart_get_handle(slot);

    if (handle == NULL) {
        return FW_FAIL;
    }
    return (R_SCI_UART_Open(handle->p_ctrl, handle->p_cfg) == FSP_SUCCESS)
            ? FW_OK : FW_FAIL;
}

size_t drv_uart_read(drv_uart_slot_e slot, uint8_t* buf, size_t size)
{
    const uart_instance_t* handle = drv_uart_get_handle(slot);

    if (handle == NULL) {
        return 0U;
    }
    /* DTC-backed read: returns immediately, completion comes via ISR callback. */
    return (R_SCI_UART_Read(handle->p_ctrl, buf, size) == FSP_SUCCESS) ? size : 0U;
}

size_t drv_uart_write(drv_uart_slot_e slot, const uint8_t* data, size_t size)
{
    const uart_instance_t* handle = drv_uart_get_handle(slot);

    if (handle == NULL) {
        return 0U;
    }
    return (R_SCI_UART_Write(handle->p_ctrl, (uint8_t*)data, size) == FSP_SUCCESS)
            ? size : 0U;
}

fw_status_t drv_uart_abort_rx(drv_uart_slot_e slot)
{
    const uart_instance_t* handle = drv_uart_get_handle(slot);

    if (handle == NULL) {
        return FW_FAIL;
    }
    return (R_SCI_UART_Abort(handle->p_ctrl, UART_DIR_RX) == FSP_SUCCESS)
            ? FW_OK : FW_FAIL;
}

/* ===========================================================================
 * ISR entry points and callback dispatch
 *
 * Pattern: ISR is a thin top-half. It only dispatches to a function pointer
 * registered by the upper layer (ddi_uart). drv_uart has zero knowledge of
 * what the upper layer will do with the event.
 * ========================================================================= */

/* FSP-mapped ISR entry for SCI0 (console). */
void isr_uart0(uart_callback_args_t* pArgs)
{
    if (pArgs->event == UART_EVENT_TX_COMPLETE) {
        if (cbConsoleTx != NULL) {
            cbConsoleTx();
        }
    } else if (pArgs->event == UART_EVENT_RX_COMPLETE) {
        if (cbConsoleRx != NULL) {
            cbConsoleRx();
        }
    }
}

/* FSP-mapped ISR entry for SCI4 (LCD).
 * RX completes via DTC, so this ISR is called once per 38-byte block - the
 * upper layer (ddi_uart) handles ring-buffer copy + re-arm + sem put. */
void isr_uart4(uart_callback_args_t* pArgs)
{
    if (pArgs->event == UART_EVENT_TX_COMPLETE) {
        if (cbLcdTx != NULL) {
            cbLcdTx();
        }
    } else if (pArgs->event == UART_EVENT_RX_COMPLETE) {
        if (cbLcdRx != NULL) {
            cbLcdRx();
        }
    }
}

void drv_uart_console_tx_register_callback(drv_uart_callback_t cb)
{
    cbConsoleTx = cb;
}

void drv_uart_console_rx_register_callback(drv_uart_callback_t cb)
{
    cbConsoleRx = cb;
}

void drv_uart_lcd_tx_register_callback(drv_uart_callback_t cb)
{
    cbLcdTx = cb;
}

void drv_uart_lcd_rx_register_callback(drv_uart_callback_t cb)
{
    cbLcdRx = cb;
}
