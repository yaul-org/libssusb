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
#define PACKET_REV_RED_DATA_SIZE        191
#define PACKET_REV_RED_TOTAL_SIZE       (PACKET_REV_RED_HEADER_SIZE +          \
    PACKET_REV_RED_DATA_SIZE)

#define PACKET_TYPE_RECEIVE_FIRST       0x01
#define PACKET_TYPE_RECEIVE_MIDDLE      0x11
#define PACKET_TYPE_RECEIVE_FINAL       0x21
#define PACKET_TYPE_SEND                0x09
#define PACKET_TYPE_SEND_EXECUTE        0x19
#define PACKET_TYPE_TEST                0x64

#define PACKET_HEADER_SEND              0x5A
#define PACKET_HEADER_RECEIVE           0xA5

#define DEVICE_TEST_TRIES 10

typedef enum {
        PACKET_RESPONSE_TYPE_RECEIVE,
        PACKET_RESPONSE_TYPE_SEND,
        PACKET_RESPONSE_TYPE_TEST
} packet_response_type_t;

typedef struct {
        uint32_t baud_rate;

        struct {
                uint32_t header_size;
                uint32_t data_size;
                uint32_t total_size;
        } packet;
} device_rev_t;

/* static ssusb_driver_error_t _driver_error = SSUSB_DRIVER_OK; */
static struct ftdi_context _ftdi_context;
static int _ftdi_error = 0;
static const device_rev_t *_device_rev = NULL;

static const device_rev_t _device_rev_red = {
        .baud_rate = REV_RED_BAUD_RATE,
        .packet = {
                .header_size = PACKET_REV_RED_HEADER_SIZE,
                .data_size   = PACKET_REV_RED_DATA_SIZE,
                .total_size  = PACKET_REV_RED_TOTAL_SIZE
        }
};

static const device_rev_t _device_rev_green = {
        .baud_rate = REV_GREEN_BAUD_RATE,
        .packet = {
                .header_size = PACKET_REV_GREEN_HEADER_SIZE,
                .data_size   = PACKET_REV_GREEN_DATA_SIZE,
                .total_size  = PACKET_REV_GREEN_TOTAL_SIZE
        }
};

static int _red_init(void);
static int _green_init(void);
static int _bluetooth_init(void);
static int _init(const device_rev_t *device_rev);

static int _deinit(void);
static int _bluetooth_deinit(void);

static int _device_revision_test(void);

static int _poll(size_t *read_len);
static int _device_read(void *buffer, size_t len);
static int _device_write(const void *buffer, size_t len);

static int _upload_execute_buffer(const void *buffer, uint32_t base_address,
    size_t len, bool execute);

static int _packet_check(const uint8_t *, packet_response_type_t response_type);

/* Helpers */
static uint8_t _packet_checksum(const uint8_t *, uint32_t);

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
        assert(false && "Not yet implemented");

        return -1;
}

static int
_init(const device_rev_t *device_rev)
{
        DEBUG_PRINTF("Enter\n");

        _device_rev = device_rev;

        if ((_ftdi_error = ftdi_init(&_ftdi_context)) < 0) {
                DEBUG_PRINTF("ftdi_init()\n");
                return -1;
        }
        _ftdi_error = ftdi_usb_open(&_ftdi_context, I_VENDOR, I_PRODUCT);
        if (_ftdi_error < 0) {
                DEBUG_PRINTF("ftdi_usb_open()\n");
                goto error;
        }
        if ((ftdi_usb_match_product(&_ftdi_context, I_PRODUCT_STRING)) < 0) {
                DEBUG_PRINTF("ftdi_usb_match_product()\n");
                goto error;
        }
        if ((_ftdi_error = ftdi_set_baudrate(&_ftdi_context, _device_rev->baud_rate)) < 0) {
                DEBUG_PRINTF("ftdi_set_baudrate()\n");
                goto error;
        }
        if ((_ftdi_error = ftdi_set_line_property(&_ftdi_context, BITS_8, STOP_BIT_2, NONE)) < 0) {
                DEBUG_PRINTF("ftdi_set_line_property()\n");
                goto error;
        }
        if ((_ftdi_error = ftdi_tcioflush(&_ftdi_context)) < 0) {
                DEBUG_PRINTF("ftdi_tcioflush()\n");
                goto error;
        }

        if ((_device_revision_test()) < 0) {
                DEBUG_PRINTF("_device_revision_test()\n");
                goto error;
        }

        return 0;

error:
        DEBUG_PRINTF("_ftdi_error: %i\n", _ftdi_error);

        if ((_ftdi_error = ftdi_usb_close(&_ftdi_context)) < 0) {
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

        if ((_ftdi_error = ftdi_tcioflush(&_ftdi_context)) < 0) {
                exit_code = -1;
                goto exit;
        }

        if ((_ftdi_error = ftdi_usb_close(&_ftdi_context)) < 0) {
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
        assert(false && "Not yet implemented");

        return -1;
}

static int
_device_revision_test(void)
{
        /* We want to use the Red LED header byte size as with this revision,
         * we'll get a valid (error) packet */
        uint8_t read_buffer[PACKET_REV_RED_HEADER_SIZE];

        /* This packet was extracted from the official tool via USB packet
         * sniffing */
        static const uint8_t packet_test[] = {
                PACKET_HEADER_SEND,
                0x07,
                PACKET_TYPE_TEST,
                0x00,
                0x18,
                0x00,
                0x00,
                0x00,
                0x83
        };

        if ((_device_write(packet_test, PACKET_REV_RED_HEADER_SIZE)) < 0) {
                DEBUG_PRINTF("_device_write error\n");
                return -1;
        }

        (void)memset(read_buffer, 0x00, sizeof(read_buffer));

        for (int tries = DEVICE_TEST_TRIES; ; tries--) {
                size_t read_len;
                if ((_poll(&read_len)) < 0) {
                        return -1;
                }

                DEBUG_PRINTF("read_len: %zu\n", read_len);
                if (read_len == PACKET_REV_RED_HEADER_SIZE) {
                        DEBUG_PRINTF("Found\n");
                        break;
                }

                usleep(RX_TIMEOUT);

                if (tries == 0) {
                        DEBUG_PRINTF("Timeout\n");
                        return -1;
                }
        }

        if ((_device_read(read_buffer, PACKET_REV_RED_HEADER_SIZE)) < 0) {
                DEBUG_PRINTF("_device_read error\n");
                return -1;
        }

        if ((_packet_check(read_buffer, PACKET_RESPONSE_TYPE_TEST)) < 0) {
                return -1;
        }

        return 0;
}

static int
_upload_buffer(const void *buffer, uint32_t base_address, size_t len)
{
        DEBUG_PRINTF("Enter\n");

        int ret;
        ret = _upload_execute_buffer(buffer, base_address, len,
            /* execute = */ false);

        DEBUG_PRINTF("Exit\n");

        return ret;
}

static int
_poll(size_t *read_len)
{
        return ftdi_buffered_poll(&_ftdi_context, read_len);
}

static int
_peek(size_t len, void *buffer, size_t *read_len)
{
        return ftdi_buffered_peek(&_ftdi_context, len, buffer, read_len);
}

static int
_device_read(void *buffer, size_t len)
{
        return ftdi_buffered_read_data(&_ftdi_context, buffer, len);
}

static int
_device_write(const void *buffer, size_t len)
{
        return ftdi_buffered_write_data(&_ftdi_context, buffer, len);
}

static int
_download_buffer(void *buffer, uint32_t base_address, size_t len)
{
#define STATE_RECEIVE_FIRST  0
#define STATE_RECEIVE_MIDDLE 1
#define STATE_RECEIVE_FINAL  2
#define STATE_FINISH         3

        DEBUG_PRINTF("Enter\n");

        /* Sanity check */
        if (buffer == NULL) {
                /* datalink_error = DATALINK_BAD_REQUEST; */
                return -1;
        }

        if (base_address == 0x00000000) {
                /* datalink_error = DATALINK_BAD_REQUEST; */
                return -1;
        }

        if (len <= 1) {
                /* datalink_error = DATALINK_BAD_REQUEST; */
                return -1;
        }

        int exit_code;
        exit_code = 0;

        /* datalink_error = DATALINK_OK; */

        uint8_t buffer_first[] = {
                PACKET_HEADER_SEND,
                0x07,
                PACKET_TYPE_RECEIVE_FIRST,
                0, /* MSB address */
                0, /* 02 address */
                0, /* 01 address */
                0, /* LSB address */
                0, /* Datalink */
                0 /* Checksum */
        };

        uint8_t buffer_middle[] = {
                PACKET_HEADER_SEND,
                0x07,
                PACKET_TYPE_RECEIVE_MIDDLE,
                0, /* MSB address */
                0, /* 02 address */
                0, /* 01 address */
                0, /* LSB address */
                0, /* Datalink */
                0 /* Checksum */
        };

        uint8_t buffer_final[] = {
                PACKET_HEADER_SEND,
                0x07,
                PACKET_TYPE_RECEIVE_FINAL,
                0, /* MSB address */
                0, /* 02 address */
                0, /* 01 address */
                0, /* LSB address */
                0, /* Length */
                0 /* Checksum */
        };

        uint8_t read_buffer[_device_rev->packet.total_size];

        int32_t state;
        state = STATE_RECEIVE_FIRST;
        uint32_t count;
        count = 0;
        uint32_t address;
        address = base_address;

        uint8_t *write_buffer;
        write_buffer = NULL;

        const bool tier_1 = len <= _device_rev->packet.data_size;
        const bool tier_2 = (len > _device_rev->packet.data_size) &&
                            (len <= (2 * _device_rev->packet.data_size));
        const bool tier_3 = len > (2 * _device_rev->packet.data_size);

        while (true) {
                switch (state) {
                case STATE_RECEIVE_FIRST:
                        DEBUG_PRINTF("%s", "State STATE_RECEIVE_FIRST\n");

                        write_buffer = &buffer_first[0];

                        if (tier_1) {
                                write_buffer[7] = len - 1;
                                state = STATE_RECEIVE_FINAL;
                        } else if (tier_2) {
                                write_buffer[7] = _device_rev->packet.data_size;
                                state = STATE_RECEIVE_FINAL;
                        } else if (tier_3) {
                                write_buffer[7] = _device_rev->packet.data_size;
                                /* The number of middle packets to send */
                                count = (len - _device_rev->packet.data_size - 1) /
                                    _device_rev->packet.data_size;

                                state = STATE_RECEIVE_MIDDLE;
                        }
                        break;
                case STATE_RECEIVE_MIDDLE:
                        write_buffer = &buffer_middle[0];

                        DEBUG_PRINTF("State STATE_RECEIVE_MIDDLE (%i)\n", count);

                        write_buffer[7] = _device_rev->packet.data_size;

                        count--;

                        if (count == 0) {
                                state = STATE_RECEIVE_FINAL;
                        }
                        break;
                case STATE_RECEIVE_FINAL:
                        DEBUG_PRINTF("State STATE_RECEIVE_FINAL\n");

                        write_buffer = &buffer_final[0];

                        if (tier_1) {
                                write_buffer[7] = 1;
                        } else if (tier_2) {
                                write_buffer[7] = len - _device_rev->packet.data_size;
                        } else if (tier_3) {
                                write_buffer[7] = (len - _device_rev->packet.data_size) %
                                    _device_rev->packet.data_size;
                                write_buffer[7] = (write_buffer[7] == 0)
                                    ? _device_rev->packet.data_size
                                    : write_buffer[7];
                        }

                        state = STATE_FINISH;
                        break;
                case STATE_FINISH:
                        goto exit;
                }

                write_buffer[3] = ADDRESS_MSB(address);
                write_buffer[4] = ADDRESS_02(address);
                write_buffer[5] = ADDRESS_01(address);
                write_buffer[6] = ADDRESS_LSB(address);

                write_buffer[8] = _packet_checksum(write_buffer,
                    _device_rev->packet.header_size - 1);

                if ((_device_write(write_buffer, _device_rev->packet.header_size)) < 0) {
                        /* DEBUG_PRINTF("datalink_error = %s\n", */
                        /*     datalink_error_strings[datalink_error]); */
                        goto error;
                }

                (void)memset(read_buffer, 0x00, _device_rev->packet.total_size);
                if ((_device_read(read_buffer, _device_rev->packet.header_size + write_buffer[7])) < 0) {
                        /* DEBUG_PRINTF("datalink_error = %s\n", */
                        /*     datalink_error_strings[datalink_error]); */
                        goto error;
                }

                if ((_packet_check(read_buffer, PACKET_RESPONSE_TYPE_RECEIVE)) < 0) {
                        /* DEBUG_PRINTF("datalink_error = %s\n", */
                        /*     datalink_error_strings[datalink_error]); */
                        goto error;
                }

                address += read_buffer[7] + 1;

                (void)memcpy(buffer, &read_buffer[_device_rev->packet.header_size - 1],
                    read_buffer[7]);
        }

        goto exit;

error:
        exit_code = -1;

exit:
        DEBUG_PRINTF("Exit\n");

        return exit_code;
}

static int
_execute_buffer(const void *buffer, uint32_t base_address, size_t len)
{
        DEBUG_PRINTF("Enter\n");

        int ret = _upload_execute_buffer(buffer, base_address, len,
            /* execute = */ true);

        DEBUG_PRINTF("Exit\n");

        return ret;
}

static int
_upload_execute_buffer(const void *buffer, uint32_t base_address,
    size_t len, bool execute)
{
        DEBUG_PRINTF("Enter\n");

        int exit_code;
        exit_code = 0;

        /* datalink_error = DATALINK_OK; */

        /* Sanity check */
        if (buffer == NULL) {
                /* datalink_error = DATALINK_BAD_REQUEST; */
                return -1;
        }

        if (base_address == 0x00000000) {
                /* datalink_error = DATALINK_BAD_REQUEST; */
                return -1;
        }

        const uint8_t *buffer_pos;
        buffer_pos = buffer;

        const void * const base_buffer = buffer;
        const uint32_t base_len = len;

        uint32_t address;
        address = base_address;

        uint8_t packet_buffer[_device_rev->packet.total_size];
        (void)memset(packet_buffer, 0x00, _device_rev->packet.total_size);
        packet_buffer[0] = PACKET_HEADER_SEND;
        packet_buffer[1] = 7 + 2;
        packet_buffer[2] = (execute) ? PACKET_TYPE_SEND_EXECUTE : PACKET_TYPE_SEND;
        packet_buffer[3] = ADDRESS_MSB(base_address);
        packet_buffer[4] = ADDRESS_02(base_address);
        packet_buffer[5] = ADDRESS_01(base_address);
        packet_buffer[6] = ADDRESS_LSB(base_address);
        packet_buffer[7] = 2;
        (void)memcpy(&packet_buffer[8], buffer_pos, 2);
        buffer_pos += 2;
        packet_buffer[_device_rev->packet.header_size + 2 - 1] =
            _packet_checksum(packet_buffer, (_device_rev->packet.header_size + 2) - 1);

        len -= 2;
        address += 2;

        uint8_t read_buffer[_device_rev->packet.total_size];
        uint8_t tmp_buffer[_device_rev->packet.total_size];

        do {
                uint8_t transfer_len;
                transfer_len = (((int32_t)len - (int32_t)_device_rev->packet.data_size) < 0)
                    ? len % _device_rev->packet.data_size
                    : _device_rev->packet.data_size;

                uint8_t *write_buffer;

                DEBUG_PRINTF("Transferring %iB (%zuB) to 0x%08X\n",
                    transfer_len, len, address);

                if (len == 0) {
                        write_buffer = packet_buffer;
                        transfer_len = 2;
                } else {
                        write_buffer = tmp_buffer;

                        (void)memset(write_buffer, 0x00, _device_rev->packet.total_size);
                        write_buffer[0] = PACKET_HEADER_SEND;
                        write_buffer[1] = (7 + transfer_len) > ((int32_t)_device_rev->packet.total_size - 2)
                            ? (int32_t)_device_rev->packet.total_size - 2
                            : 7 + transfer_len;
                        write_buffer[2] = PACKET_TYPE_SEND;
                        write_buffer[3] = ADDRESS_MSB(address);
                        write_buffer[4] = ADDRESS_02(address);
                        write_buffer[5] = ADDRESS_01(address);
                        write_buffer[6] = ADDRESS_LSB(address);
                        write_buffer[7] = transfer_len;
                        (void)memcpy(&write_buffer[8], buffer_pos, transfer_len);
                        buffer_pos += transfer_len;
                        write_buffer[_device_rev->packet.header_size + transfer_len - 1] =
                            _packet_checksum(write_buffer,
                                (_device_rev->packet.header_size + transfer_len) - 1);
                }

                if ((_device_write(write_buffer, _device_rev->packet.header_size + transfer_len)) < 0) {
                        DEBUG_PRINTF("_device_write error\n");
                        goto error;
                }

                (void)memset(read_buffer, 0x00, _device_rev->packet.total_size);
                if ((_device_read(read_buffer, _device_rev->packet.header_size)) < 0) {
                        DEBUG_PRINTF("_device_read error\n");
                        goto error;
                }

                if ((_packet_check(read_buffer, PACKET_RESPONSE_TYPE_SEND)) < 0) {
                        DEBUG_PRINTF("_packet_check error\n");
                        goto error;
                }

                len -= transfer_len;
                address += transfer_len;
        } while ((int32_t)len >= 0);

        if ((uintptr_t)base_buffer != (uintptr_t)(buffer_pos - base_len)) {
                DEBUG_PRINTF("base_buffer=%p != buffer_pos-len=%p\n",
                    (void *)base_buffer,
                    (void *)(buffer_pos - base_len));

                /* datalink_error = DATALINK_INSUFFICIENT_READ_DATA; */
                goto error;
        }

        goto exit;

error:
        exit_code = -1;

exit:
        DEBUG_PRINTF("Exit\n");

        return exit_code;
}

static uint8_t
_packet_checksum(const uint8_t *buffer, uint32_t len)
{
        DEBUG_PRINTF("Enter\n");
        DEBUG_PRINTF("Reading %iB from buffer\n", len);

        DEBUG_HEXDUMP(buffer, len);

        uint32_t checksum;
        checksum = 0;

        uint32_t buffer_idx;
        for (buffer_idx = 1; buffer_idx < len; buffer_idx++) {
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
_packet_check(const uint8_t *buffer, packet_response_type_t response_type)
{
        static const uint8_t packet_response_error[] = {
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

        static const uint8_t packet_response_send[] = {
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

        DEBUG_PRINTF("Enter\n");

        /* datalink_error = DATALINK_OK; */

        /* Check if the response packet is an error packet */
        uint32_t buffer_idx;
        bool bad_packet;

        /* XXX: Refactor to memcmp() */
        for (buffer_idx = 0;
             ((buffer_idx < _device_rev->packet.header_size) &&
                 (buffer[buffer_idx] == packet_response_error[buffer_idx]));
             buffer_idx++);

        bad_packet = buffer_idx == _device_rev->packet.header_size;
        if (bad_packet) {
                DEBUG_HEXDUMP(buffer, _device_rev->packet.header_size);

                /* If the response type is test, the error packet is valid */
                if (response_type == PACKET_RESPONSE_TYPE_TEST) {
                        /* datalink_error = DATALINK_OK; */
                        return 0;
                }

                /* datalink_error = DATALINK_ERROR_PACKET; */
                return -1;
        }

        uint8_t calc_checksum;
        uint8_t checksum;

        switch (response_type) {
        case PACKET_RESPONSE_TYPE_RECEIVE:
                calc_checksum = _packet_checksum(buffer,
                    (_device_rev->packet.header_size - 1) + buffer[7]);
                checksum = buffer[(_device_rev->packet.header_size) + buffer[7] - 1];

                if (checksum != calc_checksum) {
                        DEBUG_PRINTF("Checksum mismatch (0x%02X, 0x%02X)\n",
                            calc_checksum,
                            checksum);
                        /* datalink_error = DATALINK_CORRUPTED_PACKET; */
                        return -1;
                }
                break;
        case PACKET_RESPONSE_TYPE_SEND:
                /* XXX: Refactor to memcmp() */
                for (buffer_idx = 0;
                     ((buffer_idx < _device_rev->packet.header_size) &&
                         (buffer[buffer_idx] == packet_response_send[buffer_idx]));
                     buffer_idx++);
                bad_packet = buffer_idx != _device_rev->packet.header_size;
                if (bad_packet) {
                        DEBUG_HEXDUMP(buffer, _device_rev->packet.header_size);

                        /* datalink_error = DATALINK_ERROR_PACKET; */
                        return -1;
                }
                break;
        default:
                return -1;
        }

        /* datalink_error = DATALINK_OK; */

        return 0;
}

const ssusb_device_driver_t __device_datalink_red = {
        .name            = "datalink-red",
        .description     = "USB DataLink Red LED Revision",
        .init            = _red_init,
        .deinit          = _deinit,
        /* .error           = _error, */
        .poll            = _poll,
        .peek            = _peek,
        .read            = _device_read,
        .write           = _device_write,
        .download_buffer = _download_buffer,
        .upload_buffer   = _upload_buffer,
        .execute_buffer  = _execute_buffer,
};

const ssusb_device_driver_t __device_datalink_green = {
        .name            = "datalink-green",
        .description     = "USB DataLink Green LED Revision",
        .init            = _green_init,
        .deinit          = _deinit,
        /* .error           = _error, */
        .poll            = _poll,
        .peek            = _peek,
        .read            = _device_read,
        .write           = _device_write,
        .download_buffer = _download_buffer,
        .upload_buffer   = _upload_buffer,
        .execute_buffer  = _execute_buffer,
};

const ssusb_device_driver_t __device_datalink_bluetooth = {
        .name            = "datalink-bluetooth",
        .description     = "USB DataLink Bluetooth LED",
        .init            = _bluetooth_init,
        .deinit          = _bluetooth_deinit,
        /* .error           = _error, */
        .poll            = _poll,
        .peek            = _peek,
        .read            = _device_read,
        .write           = _device_write,
        .download_buffer = _download_buffer,
        .upload_buffer   = _upload_buffer,
        .execute_buffer  = _execute_buffer,
};
