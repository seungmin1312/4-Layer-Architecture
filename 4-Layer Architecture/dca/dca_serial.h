#ifndef DCA_SERIAL_H_
#define DCA_SERIAL_H_

#include "../tools/common.h"
#include "../ddi/ddi_uart.h"

/* ---------------------------------------------------------------------------
 * Packet protocol constants
 *
 * Domain-specific values (STX, ETX, command codes, length) are redacted in
 * this portfolio copy. Real values live in the customer-facing firmware spec.
 * ------------------------------------------------------------------------- */
#define D_PACKET_STX                  (0x00U)               /* tuning value redacted */
#define D_PACKET_ETX                  (0x00U)               /* tuning value redacted */
#define D_PACKET_MAX                  (D_COMM_DTC_BLOCK_SIZE)
#define D_PACKET_DATA_LENGTH          (D_PACKET_MAX - 5U)
#define D_PACKET_BETWEEN_STX_AND_ETX  (D_PACKET_MAX - 2U)
#define D_COMM_CHECKSUM_LEN           (1U)

#define D_RX_TIMEOUT_MS               (2000U)

typedef enum {
    E_PACKET_CMD_TX = 0x00,                             /* tuning value redacted */
    E_PACKET_CMD_RX = 0x01,                             /* tuning value redacted */
    E_PACKET_CMD_MAX,
    E_PACKET_CMD_UNKNOWN = E_PACKET_CMD_MAX
} packet_cmd_e;

typedef enum {
    E_DCA_SERIAL_RFSCREEN_STATUS_ON = 0,
    E_DCA_SERIAL_RFSCREEN_STATUS_OFF,
    E_DCA_SERIAL_RFSCREEN_STATUS_MAX,
    E_DCA_SERIAL_RFSCREEN_STATUS_UNKNOWN = E_DCA_SERIAL_RFSCREEN_STATUS_MAX
} dca_serial_rfscreen_status_e;

/* Polymorphic comm endpoint - lets the same packet codec drive UART, USB,
 * a TCP socket, or a unit-test loopback by swapping these two pointers. */
typedef struct {
    size_t (*read )(uint8_t* buf,  size_t bufsize);
    size_t (*write)(uint8_t* data, size_t datasize);
} comm_io_t;

typedef struct {
    packet_cmd_e cmd;
    size_t       size;
    uint8_t      data[D_PACKET_DATA_LENGTH];
} comm_packet_t;

bool                          dca_serial_write(comm_packet_t* packet);
bool                          dca_serial_read (comm_packet_t* packet);
void                          dca_serial_poll (void);
bool                          dca_serial_rfscreen_check(void);
dca_serial_rfscreen_status_e  dca_serial_get_rfscreen_status(void);
fw_status_t                   dca_serial_init(void);

#endif /* DCA_SERIAL_H_ */
