#include "ddi_uart.h"
#include "ddi_semaphore.h"
#include "../tools/common.h"

/* ===========================================================================
 * UART ring buffer + DTC top-half
 *
 * Design rationale
 * ----------------
 * UART RX uses DTC (data transfer controller, Renesas equivalent of DMA).
 * The CPU only sees one interrupt per DTC block - typically once per
 * received packet. Inside that ISR we:
 *
 *   1. Copy the just-arrived block into a software ring buffer.
 *   2. Immediately re-arm DTC for the next block - never block waiting.
 *   3. Release a semaphore so the parser task wakes up.
 *
 * Net effect: the ISR is ~38 byte-copies + 1 semaphore put. Everything else
 * (packet parsing, validation, dispatch) runs in task context with full
 * RTOS scheduling available.
 *
 * The polling watchdog `ddi_uart_dtc_poll()` covers the pathological case
 * where DTC stops firing (e.g. line noise corrupts the start of a block).
 * ========================================================================= */

static ring_buf_t    commBuf;
static uart_status_t uart4Status = { .rxComplete = false, .txComplete = true };
static uint8_t       dtcRxBuf[D_COMM_DTC_BLOCK_SIZE];
static uint32_t      dtcRxWatchTimer;

static size_t get_ring_buf_len(void)
{
    return (commBuf.tail - commBuf.head + D_COMM_RX_BUFSIZE) % D_COMM_RX_BUFSIZE;
}

static uint32_t read_ring_buf(uint8_t* dest, uint32_t length)
{
    uint32_t readLen = MIN(length, get_ring_buf_len());
    uint32_t i       = 0U;

    for (i = 0U; i < readLen; i++) {
        dest[i] = commBuf.rxBuffer[commBuf.head];
        commBuf.head = (commBuf.head + 1U) % D_COMM_RX_BUFSIZE;
    }
    return readLen;
}

/* ---- ISR-context handlers (top-half) ------------------------------------- */

static void on_tx_complete(void)
{
    uart4Status.txComplete = true;
}

static void on_rx_complete_dtc(void)
{
    uint32_t i        = 0U;
    uint32_t nextTail = 0U;

    /* 1) DTC block -> ring buffer (drop on overflow, never block). */
    for (i = 0U; i < D_COMM_DTC_BLOCK_SIZE; i++) {
        nextTail = (commBuf.tail + 1U) % D_COMM_RX_BUFSIZE;
        if (nextTail == commBuf.head) {
            break;                                      /* full - truncate */
        }
        commBuf.rxBuffer[commBuf.tail] = dtcRxBuf[i];
        commBuf.tail = nextTail;
    }

    /* 2) Re-arm DTC immediately for the next block. */
    (void)drv_uart_read(E_DRV_UART_LCD, dtcRxBuf, D_COMM_DTC_BLOCK_SIZE);

    /* 3) Bookkeeping + task wakeup. The task does the heavy lifting. */
    uart4Status.rxComplete = true;
    dtcRxWatchTimer = timext_start();
    (void)ddi_semaphore_put(E_DDI_SEMAPHORE_UART_RX_READY);   /* ISR-safe */
}

/* ---- Init -- everything below runs in task context ---------------------- */

void ddi_uart_ring_buf_init(void)
{
    commBuf.head = 0U;
    commBuf.tail = 0U;
}

void ddi_uart_handler_init(void)
{
    /* Register our handlers with drv_uart - this is the "upward callback"
     * registration that lets drv stay ignorant of ddi. */
    drv_uart_lcd_rx_register_callback(on_rx_complete_dtc);
    drv_uart_lcd_tx_register_callback(on_tx_complete);

    /* Prime DTC for the first block. */
    (void)drv_uart_read(E_DRV_UART_LCD, dtcRxBuf, D_COMM_DTC_BLOCK_SIZE);
    dtcRxWatchTimer = timext_start();
}

/* Watchdog poll - called by th_comm at ~150ms cadence. If we haven't seen a
 * DTC completion within the timeout, abort and re-arm. */
void ddi_uart_dtc_poll(void)
{
    if (timext_expired(dtcRxWatchTimer, D_DTC_TIMEOUT_MS)) {
        (void)drv_uart_abort_rx(E_DRV_UART_LCD);
        (void)drv_uart_read(E_DRV_UART_LCD, dtcRxBuf, D_COMM_DTC_BLOCK_SIZE);
        dtcRxWatchTimer = timext_start();
    }
}

bool ddi_uart_check_dtc_tx_complete(void)
{
    return uart4Status.txComplete;
}

bool ddi_uart_check_dtc_rx_complete(void)
{
    bool rxComplete = uart4Status.rxComplete;

    uart4Status.rxComplete = false;                     /* read clears */
    return rxComplete;
}

fw_status_t ddi_uart_open(ddi_uart_slot_e slot)
{
    if (slot >= E_DDI_UART_HANDLE_MAX) {
        return FW_FAIL;
    }
    return drv_uart_open((drv_uart_slot_e)slot);
}

size_t ddi_uart_read(ddi_uart_slot_e slot, uint8_t* buf, size_t size)
{
    (void)slot;
    return read_ring_buf(buf, (uint32_t)size);
}

size_t ddi_uart_write(ddi_uart_slot_e slot, uint8_t* data, size_t size)
{
    if (slot >= E_DDI_UART_HANDLE_MAX) {
        return 0U;
    }
    uart4Status.txComplete = false;
    return drv_uart_write((drv_uart_slot_e)slot, data, size);
}

/* ---- comm_io_t vtable bindings ------------------------------------------ */

size_t comm_read(uint8_t* buf, size_t size)
{
    return ddi_uart_read(E_DDI_LCD, buf, size);
}

size_t comm_write(uint8_t* data, size_t size)
{
    return ddi_uart_write(E_DDI_LCD, data, size);
}
