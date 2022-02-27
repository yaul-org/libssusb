/*
 * Copyright (c) 2012-2022 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <assert.h>
#include <fcntl.h>
#include <inttypes.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <ftdi.h>

#include "ftdi_ext.h"
#include "debug.h"
#include "driver.h"
#include "endianess.h"
#include "math_utilities.h"

#define RX_TIMEOUT      5000
#define TX_TIMEOUT      1000

#define I_VENDOR         0x0403
#define I_PRODUCT        0x6001
#define I_PRODUCT_STRING "MM232R USB MODULE"
#define I_SERIAL         "FTS12NIT"

/* Revision (green LED) */
#define REV_GREEN_BAUD_RATE             288000

#define PACKET_REV_GREEN_HEADER_SIZE    7
#define PACKET_REV_GREEN_DATA_SIZE      79
#define PACKET_REV_GREEN_TOTAL_SIZE     (PACKET_REV_GREEN_HEADER_SIZE +        \
    PACKET_REV_GREEN_DATA_SIZE)

/* Revision (red LED) */
#define REV_RED_BAUD_RATE               375000

#define PACKET_REV_RED_HEADER_SIZE      9
#define PACKET_REV_RED_DATA_SIZE        190
#define PACKET_REV_RED_TOTAL_SIZE       (PACKET_REV_RED_HEADER_SIZE +          \
    PACKET_REV_RED_DATA_SIZE)

#define PACKET_HEADER_SEND              0x5A
#define PACKET_HEADER_RECEIVE           0xA5

#define DEVICE_TEST_TRIES 10

typedef enum {
        PACKET_TYPE_RECEIVE_FIRST  = 0x01,
        PACKET_TYPE_RECEIVE_MIDDLE = 0x11,
        PACKET_TYPE_RECEIVE_FINAL  = 0x21,
        PACKET_TYPE_SEND           = 0x09,
        PACKET_TYPE_SEND_EXECUTE   = 0x19,
        PACKET_TYPE_TEST           = 0x64
} packet_type_t;

typedef enum {
        PACKET_RESPONSE_TYPE_RECEIVE,
        PACKET_RESPONSE_TYPE_SEND,
        PACKET_RESPONSE_TYPE_TEST
} packet_response_type_t;

typedef struct {
        uint8_t buffer[200];
        size_t size;
} packet_t;

typedef struct {
        uint32_t baud_rate;

        struct {
                uint32_t header_size;
                uint32_t data_size;
                uint32_t total_size;
                const uint8_t *response_error;
                const uint8_t *response_send;
        } packet;
} device_rev_t;

/* static ssusb_driver_error_t _driver_error = SSUSB_DRIVER_OK; */
static struct ftdi_context _ftdi_context;
static const device_rev_t *_device_rev = NULL;

static const uint8_t _red_packet_response_error[PACKET_REV_RED_HEADER_SIZE] = {
        PACKET_HEADER_RECEIVE,
        0x07,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x07
};

static const uint8_t _green_packet_response_error[PACKET_REV_GREEN_HEADER_SIZE] = {
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
};

static const uint8_t _red_packet_response_send[PACKET_REV_RED_HEADER_SIZE] = {
        PACKET_HEADER_RECEIVE,
        0x07,
        0xFF,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x06
};

static const uint8_t _green_packet_response_send[PACKET_REV_GREEN_HEADER_SIZE] = {
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00
};

static const device_rev_t _device_rev_red = {
        .baud_rate = REV_RED_BAUD_RATE,
        .packet = {
                .header_size    = PACKET_REV_RED_HEADER_SIZE,
                .data_size      = PACKET_REV_RED_DATA_SIZE,
                .total_size     = PACKET_REV_RED_TOTAL_SIZE,
                .response_error = _red_packet_response_error,
                .response_send  = _red_packet_response_send
        }
};

static const device_rev_t _device_rev_green = {
        .baud_rate = REV_GREEN_BAUD_RATE,
        .packet = {
                .header_size    = PACKET_REV_GREEN_HEADER_SIZE,
                .data_size      = PACKET_REV_GREEN_DATA_SIZE,
                .total_size     = PACKET_REV_GREEN_TOTAL_SIZE,
                .response_error = _green_packet_response_error,
                .response_send  = _green_packet_response_send
        }
};

static int _red_init(void);
static int _green_init(void);
static int _bluetooth_init(void);
static int _init(const device_rev_t *device_rev);

static int _deinit(void);
static int _bluetooth_deinit(void);

static int _device_revision_test(void);

static int _poll(size_t *read_size);
static int _usb_read(void *buffer, size_t size);
static int _usb_write(const void *buffer, size_t size);

static int _upload_execute_buffer(const void *buffer, uint32_t base_address,
    size_t size, bool execute);

static void _packet_generate(packet_type_t packet_type, packet_t *packet,
    uint32_t base_address, const void *data, size_t size);
static int _packet_xchg(packet_response_type_t response_type,
    const packet_t *packet_send, packet_t *packet_response);
static int _packet_download_xchg(packet_type_t packet_type, uint32_t address,
    void *buffer, size_t data_size);
static int _packet_response_check(packet_response_type_t response_type,
    const packet_t *packet_response);
static uint8_t _packet_checksum_generate(const uint8_t *buffer, uint32_t size);

static int
_red_init(void)
{
        DEBUG_PRINTF("Enter\n");

        return _init(&_device_rev_red);
}

static int
_green_init(void)
{
        DEBUG_PRINTF("Enter\n");

        return _init(&_device_rev_green);
}

static int
_bluetooth_init(void)
{
        return -1;
}

static int
_init(const device_rev_t *device_rev)
{
        DEBUG_PRINTF("Enter\n");

        _device_rev = device_rev;

        if ((ftdi_init(&_ftdi_context)) < 0) {
                DEBUG_PRINTF("ftdi_init()\n");
                return -1;
        }
        if ((ftdi_usb_open(&_ftdi_context, I_VENDOR, I_PRODUCT)) < 0) {
                DEBUG_PRINTF("ftdi_usb_open()\n");
                goto error;
        }
        if ((ftdi_usb_match_product(&_ftdi_context, I_PRODUCT_STRING)) < 0) {
                DEBUG_PRINTF("ftdi_usb_match_product()\n");
                goto error;
        }
        if ((ftdi_set_baudrate(&_ftdi_context, _device_rev->baud_rate)) < 0) {
                DEBUG_PRINTF("ftdi_set_baudrate()\n");
                goto error;
        }
        if ((ftdi_set_line_property(&_ftdi_context, BITS_8, STOP_BIT_2, NONE)) < 0) {
                DEBUG_PRINTF("ftdi_set_line_property()\n");
                goto error;
        }
        if ((ftdi_tcioflush(&_ftdi_context)) < 0) {
                DEBUG_PRINTF("ftdi_tcioflush()\n");
                goto error;
        }

        if ((_device_revision_test()) < 0) {
                DEBUG_PRINTF("_device_revision_test()\n");
                goto error;
        }

        return 0;

error:
        if ((ftdi_usb_close(&_ftdi_context)) < 0) {
                return -1;
        }

        ftdi_deinit(&_ftdi_context);

        return -1;
}

static int
_deinit(void)
{
        DEBUG_PRINTF("Enter\n");

        _device_rev = NULL;

        int exit_code;
        exit_code = 0;

        if ((ftdi_tcioflush(&_ftdi_context)) < 0) {
                exit_code = -1;
                goto exit;
        }

        if ((ftdi_usb_close(&_ftdi_context)) < 0) {
                exit_code = -1;
                goto exit;
        }

exit:
        ftdi_deinit(&_ftdi_context);

        return exit_code;
}

static int
_bluetooth_deinit(void)
{
        return -1;
}

static int
_device_revision_test(void)
{
        /* We want to use the Red LED header byte size as with this revision,
         * we'll get a valid (error) packet */
        packet_t packet;

        _packet_generate(PACKET_TYPE_TEST, &packet, 0x00000000, NULL, 0);

        if ((_usb_write(packet.buffer, PACKET_REV_RED_HEADER_SIZE)) < 0) {
                DEBUG_PRINTF("_device_write error\n");
                return -1;
        }

        for (int tries = DEVICE_TEST_TRIES; ; tries--) {
                size_t read_size;
                if ((_poll(&read_size)) < 0) {
                        return -1;
                }

                DEBUG_PRINTF("read_size: %zu\n", read_size);
                if (read_size == PACKET_REV_RED_HEADER_SIZE) {
                        DEBUG_PRINTF("Found\n");
                        break;
                }

                usleep(RX_TIMEOUT);

                if (tries == 0) {
                        DEBUG_PRINTF("Timeout\n");
                        return -1;
                }
        }

        if ((_usb_read(packet.buffer, PACKET_REV_RED_HEADER_SIZE)) < 0) {
                DEBUG_PRINTF("_device_read error\n");
                return -1;
        }

        if ((_packet_response_check(PACKET_RESPONSE_TYPE_TEST, &packet)) < 0) {
                return -1;
        }

        return 0;
}

static int
_upload_buffer(const void *buffer, uint32_t base_address, size_t size)
{
        DEBUG_PRINTF("Enter\n");

        int ret;
        ret = _upload_execute_buffer(buffer, base_address, size,
            /* execute = */ false);

        DEBUG_PRINTF("Exit\n");

        return ret;
}

static ssusb_driver_error_t
_error(void)
{
        return SSUSB_DRIVER_OK;
}

static int
_poll(size_t *read_size)
{
        return ftdi_buffered_poll(&_ftdi_context, read_size);
}

static int
_peek(size_t size, void *buffer, size_t *read_size)
{
        return ftdi_buffered_peek(&_ftdi_context, size, buffer, read_size);
}

static int
_usb_read(void *buffer, size_t size)
{
        DEBUG_PRINTF("[1;32mReading %zuB[m\n", size);
        return ftdi_buffered_read_data(&_ftdi_context, buffer, size);
}

static int
_usb_write(const void *buffer, size_t size)
{
        DEBUG_PRINTF("[1;32mWriting %zuB[m\n", size);

        return ftdi_buffered_write_data(&_ftdi_context, buffer, size);
}

static int
_download_buffer(void *buffer, uint32_t base_address, size_t size)
{
        DEBUG_PRINTF("Enter\n");

        if (size < 3) {
                return -1;
        }

        const size_t packet_data_size = _device_rev->packet.data_size;

        /* Try to split the size into three parts, with the bulk of it for the
         * "middle" packets */
        const size_t size_div = size / 3;
        const size_t size_remainder = size % 3;
        const size_t div_remainder = max(0U, size_div - packet_data_size);

        const size_t first_size = min(size_div, packet_data_size);

        const size_t middle_size = size_div + size_remainder + div_remainder + div_remainder;
        const uint32_t middle_count = ((middle_size + (packet_data_size - 1)) / packet_data_size);

        const size_t final_size = min(size_div, packet_data_size);

        uint32_t address;
        address = base_address;

        uint8_t *buffer_pos;
        buffer_pos = buffer;

        if ((_packet_download_xchg(PACKET_TYPE_RECEIVE_FIRST, address, buffer_pos, first_size)) < 0) {
                goto error;
        }

        address += first_size;
        buffer_pos += first_size;

        for (uint32_t i = 0, remainder_size = middle_size; i < middle_count; i++) {
                const size_t data_size = min(remainder_size, packet_data_size);

                if ((_packet_download_xchg(PACKET_TYPE_RECEIVE_MIDDLE, address, buffer_pos, data_size)) < 0) {
                        goto error;
                }

                address += data_size;
                buffer_pos += data_size;
                remainder_size -= data_size;
        }

        if ((_packet_download_xchg(PACKET_TYPE_RECEIVE_FINAL, address, buffer_pos, final_size)) < 0) {
                goto error;
        }

        DEBUG_PRINTF("Exit\n");

        return 0;

error:
        DEBUG_PRINTF("Exit\n");

        return -1;
}

static int
_execute_buffer(const void *buffer, uint32_t base_address, size_t size)
{
        DEBUG_PRINTF("Enter\n");

        int ret = _upload_execute_buffer(buffer, base_address, size,
            /* execute = */ true);

        DEBUG_PRINTF("Exit\n");

        return ret;
}

static int
_upload_execute_buffer(const void *buffer, uint32_t base_address,
    size_t size, bool execute)
{
        DEBUG_PRINTF("Enter\n");

        static packet_t packet;
        static packet_t packet_response;

        const uint8_t *buffer_pos;
        buffer_pos = buffer;

        uint32_t address;
        address = base_address;

        /* Send the first two bytes last (and generate an execute packet). The
         * size of an SH-2 instruction is 2-bytes */
        if (execute) {
                size -= 2;
                address += 2;
                buffer_pos += 2;
        }

        while (size != 0) {
                const size_t transfer_size = min(size, _device_rev->packet.data_size);

                _packet_generate(PACKET_TYPE_SEND, &packet, address, buffer_pos, transfer_size);

                DEBUG_PRINTF("Transferring packet (%zuB) (data: %zuB -> 0x%08X)\n", packet.size, transfer_size, address);

                if ((_packet_xchg(PACKET_RESPONSE_TYPE_SEND, &packet, &packet_response)) < 0) {
                        goto error;
                }

                size -= transfer_size;
                address += transfer_size;
                buffer_pos += transfer_size;
        }

        if (execute) {
                _packet_generate(PACKET_TYPE_SEND_EXECUTE, &packet, base_address, buffer, 2);

                if ((_packet_xchg(PACKET_RESPONSE_TYPE_SEND, &packet, &packet_response)) < 0) {
                        goto error;
                }
        }

        DEBUG_PRINTF("Exit\n");

        return 0;

error:
        DEBUG_PRINTF("Exit\n");

        return -1;
}

static void
_packet_generate(packet_type_t packet_type, packet_t *packet,
    uint32_t base_address, const void *data, size_t size)
{
        (void)memset(packet, 0, sizeof(packet->buffer));

        packet->buffer[0] = PACKET_HEADER_SEND;
        packet->buffer[2] = packet_type;
        packet->buffer[3] = ADDRESS_MSB(base_address);
        packet->buffer[4] = ADDRESS_02(base_address);
        packet->buffer[5] = ADDRESS_01(base_address);
        packet->buffer[6] = ADDRESS_LSB(base_address);
        packet->buffer[7] = size;

        switch (packet_type) {
        case PACKET_TYPE_RECEIVE_FIRST:
        case PACKET_TYPE_RECEIVE_MIDDLE:
        case PACKET_TYPE_RECEIVE_FINAL:
                packet->buffer[1] = 0x07;
                packet->buffer[8] = _packet_checksum_generate(packet->buffer, _device_rev->packet.header_size - 1);

                packet->size = _device_rev->packet.header_size;
                break;
        case PACKET_TYPE_SEND:
        case PACKET_TYPE_SEND_EXECUTE:
                packet->buffer[1] = min(7 + size, _device_rev->packet.total_size);

                /* Copy data */
                if (data != NULL) {
                        (void)memcpy(&packet->buffer[8], data, size);
                }

                /* Generate checksum at last byte */
                packet->buffer[_device_rev->packet.header_size + size - 1] =
                    _packet_checksum_generate(packet->buffer, (_device_rev->packet.header_size + size) - 1);

                packet->size = _device_rev->packet.header_size + size + 1;
                break;
        case PACKET_TYPE_TEST:
                packet->buffer[1] = 0x07;
                packet->buffer[3] = 0x00;
                packet->buffer[4] = 0x18;
                packet->buffer[5] = 0x00;
                packet->buffer[6] = 0x00;
                packet->buffer[7] = 0x00;
                packet->buffer[8] = 0x83; /* Hard coded checksum */

                packet->size = _device_rev->packet.header_size;
                break;
        }
}

static int
_packet_xchg(packet_response_type_t response_type, const packet_t *packet_send,
    packet_t *packet_response)
{
        DEBUG_PRINTF("Enter\n");

        if ((_usb_write(packet_send->buffer, packet_send->size)) < 0) {
                DEBUG_PRINTF("_device_write error\n");
                return -1;
        }

        switch (response_type) {
        case PACKET_RESPONSE_TYPE_RECEIVE:
                packet_response->size = packet_send->buffer[7] + _device_rev->packet.header_size;
                break;
        case PACKET_RESPONSE_TYPE_SEND:
        case PACKET_RESPONSE_TYPE_TEST:
                packet_response->size = _device_rev->packet.header_size;
                break;
        }

        if ((_usb_read(packet_response->buffer, packet_response->size)) < 0) {
                DEBUG_PRINTF("_device_read error\n");
                return -1;
        }

        if ((_packet_response_check(response_type, packet_response)) < 0) {
                DEBUG_PRINTF("_packet_check error\n");
                return -1;
        }

        DEBUG_PRINTF("Exit\n");

        return 0;
}

static int
_packet_download_xchg(packet_type_t packet_type, uint32_t address, void *buffer,
    size_t data_size)
{
        static packet_t packet;
        static packet_t packet_response;

        _packet_generate(packet_type, &packet, address, NULL, data_size);

        DEBUG_HEXDUMP(packet.buffer, packet.size);

        if ((_packet_xchg(PACKET_RESPONSE_TYPE_RECEIVE, &packet, &packet_response)) < 0) {
                return -1;
        }

        const void * const data =
            &packet_response.buffer[_device_rev->packet.header_size - 1];

        (void)memcpy(buffer, data, data_size);

        return 0;
}

static uint8_t
_packet_checksum_generate(const uint8_t *buffer, uint32_t size)
{
        DEBUG_PRINTF("Enter\n");
        DEBUG_PRINTF("Reading %iB from buffer\n", size);

        DEBUG_HEXDUMP(buffer, size);

        uint32_t checksum;
        checksum = 0;

        uint32_t buffer_idx;
        for (buffer_idx = 1; buffer_idx < size; buffer_idx++) {
#ifdef DEBUG
                if (buffer_idx < _device_rev->packet.header_size) {
                        DEBUG_PRINTF("B[%02i]\n", buffer_idx);
                }
#endif /* DEBUG */

                checksum += (uint32_t)buffer[buffer_idx];
        }

        DEBUG_PRINTF("8-bit value: 0x%02X (%i)\n", (uint8_t)checksum,
            (uint8_t)checksum);
        DEBUG_PRINTF("32-bit value: 0x%02X (%i)\n", checksum, checksum);

        return (uint8_t)checksum;
}

static int
_packet_response_check(packet_response_type_t response_type,
    const packet_t *packet_response)
{
        DEBUG_PRINTF("Enter\n");

        /* Check if the response packet is an error packet */
        DEBUG_HEXDUMP(packet_response->buffer, packet_response->size);
        DEBUG_HEXDUMP(_device_rev->packet.response_error, _device_rev->packet.header_size);

        if ((memcmp(packet_response->buffer, _device_rev->packet.response_error, _device_rev->packet.header_size)) == 0) {
                /* If the response type is test, the error packet is valid */
                if (response_type == PACKET_RESPONSE_TYPE_TEST) {
                        return 0;
                }

                return -1;
        }

        uint8_t calc_checksum;
        uint8_t checksum;

        switch (response_type) {
        case PACKET_RESPONSE_TYPE_RECEIVE:
                calc_checksum = _packet_checksum_generate(packet_response->buffer, (_device_rev->packet.header_size - 1) + packet_response->buffer[7]);
                checksum = packet_response->buffer[_device_rev->packet.header_size + packet_response->buffer[7] - 1];

                if (checksum != calc_checksum) {
                        DEBUG_PRINTF("Checksum mismatch (0x%02X, 0x%02X)\n", calc_checksum, checksum);
                        return -1;
                }
                break;
        case PACKET_RESPONSE_TYPE_SEND:
                if ((memcmp(packet_response->buffer, _device_rev->packet.response_send, _device_rev->packet.header_size)) != 0) {
                        return -1;
                }
                break;
        default:
                return -1;
        }

        return 0;
}

const ssusb_device_driver_t device_datalink_red = {
        .name            = "datalink-red",
        .description     = "USB DataLink Red LED Revision",
        .init            = _red_init,
        .deinit          = _deinit,
        .error           = _error,
        .poll            = _poll,
        .peek            = _peek,
        .read            = _usb_read,
        .write           = _usb_write,
        .download_buffer = _download_buffer,
        .upload_buffer   = _upload_buffer,
        .execute_buffer  = _execute_buffer,
};

const ssusb_device_driver_t device_datalink_green = {
        .name            = "datalink-green",
        .description     = "USB DataLink Green LED Revision",
        .init            = _green_init,
        .deinit          = _deinit,
        .error           = _error,
        .poll            = _poll,
        .peek            = _peek,
        .read            = _usb_read,
        .write           = _usb_write,
        .download_buffer = _download_buffer,
        .upload_buffer   = _upload_buffer,
        .execute_buffer  = _execute_buffer,
};

const ssusb_device_driver_t device_datalink_bluetooth = {
        .name            = "datalink-bluetooth",
        .description     = "USB DataLink Bluetooth LED",
        .init            = _bluetooth_init,
        .deinit          = _bluetooth_deinit,
        .error           = _error,
        .poll            = _poll,
        .peek            = _peek,
        .read            = NULL,
        .write           = NULL,
        .download_buffer = _download_buffer,
        .upload_buffer   = _upload_buffer,
        .execute_buffer  = _execute_buffer,
};
