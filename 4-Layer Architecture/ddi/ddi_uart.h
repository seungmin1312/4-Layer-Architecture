#ifndef DDI_UART_H_
#define DDI_UART_H_

#include "../tools/fw_types.h"
#include "../drv/drv_uart.h"

typedef void (*ddi_uart_callback_t)(void);

/* DTC block size: one packet's worth of bytes per DTC completion. */
#define D_COMM_DTC_BLOCK_SIZE  (38U)
#define D_COMM_RX_BUFSIZE      (D_COMM_DTC_BLOCK_SIZE * 5U)  /* 5-block ring */
#define D_DTC_TIMEOUT_MS       (500U)

typedef enum {
    E_DDI_CONSOLE,
    E_DDI_LCD,
    E_DDI_UART_HANDLE_MAX,
    E_DDI_UART_HANDLE_UNKNOWN = E_DDI_UART_HANDLE_MAX
} ddi_uart_slot_e;

/* Ring buffer used to bridge the ISR-fed DTC block buffer to the task that
 * parses messages. head/tail are touched from both ISR and task contexts, so
 * the indices are modulo-bumped under the assumption of single producer
 * (ISR) and single consumer (task) - the classic SPSC pattern. */
typedef struct {
    uint8_t  rxBuffer[D_COMM_RX_BUFSIZE];
    uint32_t head;
    uint32_t tail;
} ring_buf_t;

typedef struct {
    volatile bool rxComplete;
    volatile bool txComplete;
} uart_status_t;

fw_status_t ddi_uart_open(ddi_uart_slot_e slot);

void ddi_uart_ring_buf_init(void);
void ddi_uart_handler_init (void);
void ddi_uart_dtc_poll     (void);
bool ddi_uart_check_dtc_tx_complete(void);
bool ddi_uart_check_dtc_rx_complete(void);

size_t ddi_uart_read (ddi_uart_slot_e slot, uint8_t* buf,  size_t size);
size_t ddi_uart_write(ddi_uart_slot_e slot, uint8_t* data, size_t size);

/* Thin polymorphic read/write used by comm_io_t vtable in dca_serial. */
size_t comm_read (uint8_t* buf,  size_t size);
size_t comm_write(uint8_t* data, size_t size);

#endif /* DDI_UART_H_ */
