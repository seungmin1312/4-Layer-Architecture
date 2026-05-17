#include "dca_serial.h"

/* ===========================================================================
 * dca_serial - packet codec + RX-timeout policy
 *
 * Showcases two things:
 *
 *  1. `comm_io_t` vtable. The packet encoder/decoder works against a
 *     pair of read/write function pointers, not directly against ddi_uart.
 *     Swap the vtable and the same codec runs over USB / TCP / loopback.
 *
 *  2. RX-timeout-based screen-on/screen-off policy. We don't poll busy-wait
 *     anywhere - state is derived from a 1-shot timer reset by the DTC RX
 *     completion bookkeeping. The policy is a pure function of "was there
 *     recent RX activity", computed in task context.
 *
 * All concrete protocol values (STX, ETX, length) are redacted in dca_serial.h.
 * ========================================================================= */

static dca_serial_rfscreen_status_e rfScreenStatus = E_DCA_SERIAL_RFSCREEN_STATUS_OFF;
static comm_io_t                    comm;

static void send_packet(uint8_t* buf, size_t size)
{
    comm.write(buf, size);
}

/* Frame layout (redacted constants):
 *   [STX][CMD][LEN][DATA...][CHKSUM][ETX]
 * Checksum is XOR over CMD + LEN + DATA (i.e. everything between STX and CHKSUM).
 */
static uint32_t encode_packet(uint8_t* buf, comm_packet_t* packet)
{
    size_t pos = 0U;

    buf[pos] = D_PACKET_STX;
    pos++;

    if ((packet->cmd < E_PACKET_CMD_TX) || (packet->cmd > E_PACKET_CMD_RX)) {
        error("invalid cmd=%u", packet->cmd);
        return 0U;
    }
    buf[pos] = (uint8_t)packet->cmd;
    pos++;

    if (packet->size != D_PACKET_DATA_LENGTH) {
        error("invalid data length=%u", packet->size);
        return 0U;
    }
    buf[pos] = (uint8_t)packet->size;
    pos++;

    memcpy(&buf[pos], packet->data, packet->size);
    pos += packet->size;

    buf[pos] = get_xor_checksum(&buf[1], pos - 1U);     /* CMD + LEN + DATA */
    pos++;
    buf[pos] = D_PACKET_ETX;
    pos++;

    return (uint32_t)pos;
}

static size_t read_packet(uint8_t* buf)
{
    uint8_t stx     = 0U;
    uint8_t etx     = 0U;
    size_t  readLen = 0U;

    /* 1) Search STX */
    if ((comm.read(&stx, 1U) == 0U) || (stx != D_PACKET_STX)) {
        return 0U;
    }

    /* 2) Read CMD + LEN + DATA + CHKSUM in one go */
    readLen = comm.read(buf, D_PACKET_BETWEEN_STX_AND_ETX);
    if (readLen != D_PACKET_BETWEEN_STX_AND_ETX) {
        return 0U;
    }

    /* 3) Validate ETX */
    if ((comm.read(&etx, 1U) == 0U) || (etx != D_PACKET_ETX)) {
        return 0U;
    }

    return readLen;
}

static bool decode_packet(comm_packet_t* packet, uint8_t* buf, size_t bufSize)
{
    uint8_t recv = 0U;
    uint8_t calc = 0U;

    if ((packet == NULL) || (buf == NULL)) {
        return false;
    }
    if (bufSize < (2U + D_COMM_CHECKSUM_LEN)) {
        return false;
    }

    packet->cmd  = (packet_cmd_e)buf[0];
    packet->size = buf[1];
    if (packet->size != D_PACKET_DATA_LENGTH) {
        return false;
    }
    if (bufSize < (2U + packet->size + D_COMM_CHECKSUM_LEN)) {
        return false;
    }

    /* Checksum: XOR over CMD + LEN + DATA (everything before the checksum byte). */
    recv = buf[bufSize - D_COMM_CHECKSUM_LEN];
    calc = get_xor_checksum(buf, bufSize - D_COMM_CHECKSUM_LEN);
    if (recv != calc) {
        return false;
    }

    memcpy(packet->data, buf + 2, packet->size);
    return true;
}

bool dca_serial_write(comm_packet_t* packet)
{
    static uint8_t sendBuf[D_PACKET_MAX];
    size_t         sendLen = 0U;

    memset(sendBuf, 0, sizeof(sendBuf));

    if (ddi_uart_check_dtc_tx_complete() == false) {
        return false;
    }

    sendLen = encode_packet(sendBuf, packet);
    if (sendLen == 0U) {
        return false;
    }
    send_packet(sendBuf, sendLen);
    return true;
}

bool dca_serial_read(comm_packet_t* packet)
{
    static uint8_t readBuf[D_PACKET_MAX];
    size_t         readLen = 0U;

    memset(readBuf, 0, sizeof(readBuf));

    readLen = read_packet(readBuf);
    if (readLen == 0U) {
        return false;
    }
    memset(packet, 0, sizeof(*packet));
    return decode_packet(packet, readBuf, readLen);
}

/* RX-timeout-based screen state policy.
 *
 * If we haven't seen any RX activity for D_RX_TIMEOUT_MS, treat the link
 * as down -> screen status OFF (in the original project this gated RF
 * output as a safety interlock).
 */
bool dca_serial_rfscreen_check(void)
{
    static uint32_t                     rxWatchTimer = UINT32_MAX;
    static bool                         prevTimedOut = true;
    bool                                timedOut     = true;
    dca_serial_rfscreen_status_e        prev         = rfScreenStatus;

    if (ddi_uart_check_dtc_rx_complete()) {
        rxWatchTimer = timext_start();
    }

    timedOut = (rxWatchTimer == UINT32_MAX) || timext_expired(rxWatchTimer, D_RX_TIMEOUT_MS);

    if (timedOut) {
        rfScreenStatus = E_DCA_SERIAL_RFSCREEN_STATUS_OFF;
    } else {
        rfScreenStatus = E_DCA_SERIAL_RFSCREEN_STATUS_ON;
    }

    if (timedOut != prevTimedOut) {
        info("rfscreen %s", timedOut ? "OFF (rx idle)" : "ON (rx active)");
    }
    prevTimedOut = timedOut;

    return (prev != rfScreenStatus);
}

dca_serial_rfscreen_status_e dca_serial_get_rfscreen_status(void)
{
    return rfScreenStatus;
}

void dca_serial_poll(void)
{
    ddi_uart_dtc_poll();
}

fw_status_t dca_serial_init(void)
{
    if (ddi_uart_open(E_DDI_LCD) != FW_OK) {
        return FW_FAIL;
    }

    /* Bind the codec to ddi_uart's read/write. The codec doesn't know which
     * physical channel it's talking to - this is the single point where that
     * choice is made. */
    comm.read  = comm_read;
    comm.write = comm_write;

    ddi_uart_ring_buf_init();
    ddi_uart_handler_init();
    return FW_OK;
}
