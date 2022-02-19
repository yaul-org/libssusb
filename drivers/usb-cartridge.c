/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

/*
 * Copyright (c) 2012, 2013, 2015, Anders Montonen
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <assert.h>
#include <fcntl.h>
#include <inttypes.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "debug.h"
#include "driver.h"
#include "shared.h"
#include "crc.h"
#include "ring_buffer.h"
#include "fifo.h"

#include <ftdi.h>

#define RX_TIMEOUT      5000
#define TX_TIMEOUT      1000

#define I_VENDOR        0x0403
#define I_PRODUCT       0x6001
#define I_SERIAL        "AL00P4JX"

typedef enum {
        CMD_DOWNLOAD = 1,
        CMD_UPLOAD,
        CMD_EXECUTE,
        CMD_GET_BUFFER_ADDRESS,
        CMD_COPY_EXECUTE,
        CMD_EXECUTE_EXT
} protocol_command_t;

static ssusb_driver_error_t _driver_error = SSUSB_DRIVER_OK;
static ring_buffer_t _read_ringbuffer;
static struct ftdi_context _ftdi_ctx;
static int _ftdi_error = 0;

static int _init(void);
static int _deinit(void);

/* In x86_64-w64-mingw32/include/fcntl.h, there is a conflict with a previous
 * declaration */
static int _usb_cart_read(void *buffer, size_t len);
static int _usb_cart_send(const void *buffer, size_t len);

static int _upload_execute_buffer(const void *buffer, uint32_t base_address,
    size_t len, bool execute);

static int _device_read(void *buffer, size_t len, bool block);
static int _device_write(const void *buffer, size_t len);

/* Helpers */
static int _command_send(protocol_command_t command, uint32_t address, size_t len);
static int _checksum_receive(const void *buffer, size_t len);
static int _checksum_send(const void *buffer, size_t len);

static int
_init(void)
{
        DEBUG_PRINTF("Enter\n");

#define USB_READ_PACKET_SIZE    (64 * 1024)
#define USB_WRITE_PACKET_SIZE   (4 * 1024)
#define USB_PAYLOAD(x)          ((x) - (((x) / 64) * 2))
#define READ_PAYLOAD_SIZE       (USB_PAYLOAD(USB_READ_PACKET_SIZE))
#define WRITE_PAYLOAD_SIZE      (USB_PAYLOAD(USB_WRITE_PACKET_SIZE))

        if ((_ftdi_error = ftdi_init(&_ftdi_ctx)) < 0) {
                DEBUG_PRINTF("ftdi_init()\n");
                goto error;
        }
        _ftdi_error = ftdi_usb_open(&_ftdi_ctx, I_VENDOR, I_PRODUCT);
        if (_ftdi_error < 0) {
                DEBUG_PRINTF("ftdi_usb_open()\n");
                goto error;
        }
        if ((_ftdi_error = ftdi_tcioflush(&_ftdi_ctx)) < 0) {
                DEBUG_PRINTF("ftdi_tcioflush()\n");
                goto error;
        }
        if ((_ftdi_error = ftdi_read_data_set_chunksize(&_ftdi_ctx,
                    USB_READ_PACKET_SIZE)) < 0) {
                DEBUG_PRINTF("ftdi_read_data_set_chunksize()\n");
                goto error;
        }
        if ((_ftdi_error = ftdi_write_data_set_chunksize(&_ftdi_ctx,
                    USB_WRITE_PACKET_SIZE)) < 0) {
                DEBUG_PRINTF("ftdi_write_data_set_chunksize()\n");
                goto error;
        }
        if ((_ftdi_error = ftdi_set_bitmode(&_ftdi_ctx, 0x00, BITMODE_RESET)) < 0) {
                DEBUG_PRINTF("ftdi_set_bitmode()\n");
                goto error;
        }

        ring_buffer_init(&_read_ringbuffer, USB_READ_PACKET_SIZE);

        return 0;

error:
        DEBUG_PRINTF("_ftdi_error: %i\n", _ftdi_error);

        if ((_ftdi_error = ftdi_usb_close(&_ftdi_ctx)) < 0) {
                return -1;
        }

        return -1;
}

static int
_deinit(void)
{
        DEBUG_PRINTF("Enter\n");

        int exit_code;
        exit_code = 0;

        if ((_ftdi_error = ftdi_tcioflush(&_ftdi_ctx)) < 0) {
                exit_code = -1;
                goto exit;
        }

        if ((_ftdi_error = ftdi_usb_close(&_ftdi_ctx)) < 0) {
                exit_code = -1;
                goto exit;
        }

exit:
        ring_buffer_deinit(&_read_ringbuffer);

        ftdi_deinit(&_ftdi_ctx);

        return exit_code;
}

static ssusb_driver_error_t
_error(void)
{
        return _driver_error;
}

static int
_device_read(void *buffer, size_t len, bool block)
{
        DEBUG_PRINTF("Enter\n");
        DEBUG_PRINTF("Request read of %iB\n", len);

        _driver_error = SSUSB_DRIVER_OK;

        uint8_t *buffer_pos;
        buffer_pos = buffer;

        const ring_buffer_size_t ringbuffer_size =
            ring_buffer_count(&_read_ringbuffer);
        const uint32_t preread_amount = min(ringbuffer_size, len);

        ring_buffer_dequeue_arr(&_read_ringbuffer, buffer_pos, preread_amount);

        buffer_pos += preread_amount;

        while (((uintptr_t)buffer_pos - (uintptr_t)buffer) < len) {
                DEBUG_PRINTF("Call to ftdi_read_data(%i)\n", len);

                int read_amount;
                if ((read_amount = ftdi_read_data(&_ftdi_ctx, buffer_pos, len)) < 0) {
                        _ftdi_error = read_amount;
                        return -1;
                }

                buffer_pos += read_amount;

                DEBUG_PRINTF("Read %iB\n", read_amount);

                if (!block) {
                        break;
                }
        }

        DEBUG_PRINTF("%iB read\n", (uintptr_t)buffer_pos - (uintptr_t)buffer);

        return 0;
}

static int
_device_write(const void *buffer, size_t len)
{
        DEBUG_PRINTF("Enter\n");
        DEBUG_PRINTF("Writing %iB\n", len);

        _driver_error = SSUSB_DRIVER_OK;

        uint32_t written;
        written = 0;

        while ((len - written) > 0) {
                int amount;
                if ((amount = ftdi_write_data(&_ftdi_ctx, buffer, len)) < 0) {
                        _ftdi_error = amount;
                        return -1;
                }

                written += amount;
        }

        DEBUG_PRINTF("%iB written\n", len);

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
_poll(size_t *read)
{
        _driver_error = SSUSB_DRIVER_OK;

        static uint8_t read_buffer[READ_PAYLOAD_SIZE];

        (void)memset(read_buffer, 0, READ_PAYLOAD_SIZE);

        *read = 0;

        int amount;
        if ((amount = ftdi_read_data(&_ftdi_ctx, read_buffer, READ_PAYLOAD_SIZE)) < 0) {
                _ftdi_error = SSUSB_DRIVER_DEVICE_ERROR;
                return -1;
        }

        ring_buffer_queue_arr(&_read_ringbuffer, read_buffer, amount);

        *read = ring_buffer_count(&_read_ringbuffer);

        return 0;
}

static int
_fifo_alloc(struct fifo **fifo)
{
        _driver_error = SSUSB_DRIVER_OK;

        *fifo = malloc(sizeof(struct fifo));
        if (*fifo == NULL) {
                _driver_error = SSUSB_DRIVER_INSUFFICIENT_MEMORY;
                return -1;
        }

        (*fifo)->buffer = malloc(USB_READ_PACKET_SIZE);
        if ((*fifo)->buffer == NULL) {
                _driver_error = SSUSB_DRIVER_INSUFFICIENT_MEMORY;
                return -1;
        }

        (void)memset((void *)(*fifo)->buffer, 0, USB_READ_PACKET_SIZE);

        (*fifo)->size = 0;

        return 0;
}

static int
_fifo_free(struct fifo *fifo)
{
        _driver_error = SSUSB_DRIVER_OK;

        if (fifo == NULL) {
                _driver_error = SSUSB_DRIVER_PARAM_NULL;
                return -1;
        }

        free((void *)fifo->buffer);

        fifo->buffer = NULL;
        fifo->size = 0;

        free(fifo);

        return 0;
}

static int
_peek(struct fifo *fifo)
{
        _driver_error = SSUSB_DRIVER_OK;

        if (fifo == NULL) {
                _driver_error = SSUSB_DRIVER_PARAM_NULL;
                return -1;
        }

        (void)memset((void *)fifo->buffer, 0, USB_READ_PACKET_SIZE);

        const ring_buffer_size_t ringbuffer_size =
            ring_buffer_count(&_read_ringbuffer);

        for (int32_t i = ringbuffer_size - 1; i >= 0; i--) {
                uint8_t * const buffer_pos = (uint8_t *)&fifo->buffer[i];

                ring_buffer_peek(&_read_ringbuffer, buffer_pos, i);
        }

        fifo->size = ringbuffer_size;

        return 0;
}

static int
_usb_cart_read(void *buffer, size_t len)
{
        DEBUG_PRINTF("Enter\n");

        int exit_code;
        exit_code = 0;

        _driver_error = SSUSB_DRIVER_OK;

        /* Sanity check */
        if (buffer == NULL) {
                _driver_error = SSUSB_DRIVER_BAD_REQUEST;
                goto error;
        }

        if ((_device_read(buffer, len, /* block = */ true)) < 0) {
                goto error;
        }

        goto exit;

error:
        exit_code = -1;

        /* (void)printf("ERROR: %i %s\n", __LINE__, _error_strings[_error]); */

exit:
        DEBUG_PRINTF("Exit\n");

        return exit_code;
}

static int
_usb_cart_send(const void *buffer, size_t len)
{
        DEBUG_PRINTF("Enter\n");

        int exit_code;
        exit_code = 0;

        _driver_error = SSUSB_DRIVER_OK;

        if (buffer == NULL) {
                _driver_error = SSUSB_DRIVER_BAD_REQUEST;
                goto error;
        }

        if ((_device_write(buffer, len)) < 0) {
                goto error;
        }

        goto exit;

error:
        exit_code = -1;

        /* (void)printf("ERROR: %i %s\n", __LINE__, _error_strings[_error]); */

exit:
        DEBUG_PRINTF("Exit\n");

        return exit_code;
}

static int
_download_buffer(void *buffer, uint32_t base_address, size_t len)
{
        DEBUG_PRINTF("Enter\n");

        int exit_code;
        exit_code = 0;

        _driver_error = SSUSB_DRIVER_OK;

        /* Sanity check */
        if (buffer == NULL) {
                _driver_error = SSUSB_DRIVER_BAD_REQUEST;
                goto error;
        }

        if (base_address == 0x00000000) {
                _driver_error = SSUSB_DRIVER_BAD_REQUEST;
                goto error;
        }

        if ((_command_send(CMD_DOWNLOAD, base_address, len)) < 0) {
                goto error;
        }

        if ((_device_read(buffer, len, true)) < 0) {
                goto error;
        }

        if ((_checksum_receive(buffer, len)) < 0) {
                goto error;
        }

        goto exit;

error:
        exit_code = -1;

        /* (void)printf("ERROR: %i %s\n", __LINE__, _error_strings[_error]); */

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

        _driver_error = SSUSB_DRIVER_OK;

        /* Sanity check */
        if (buffer == NULL) {
                _driver_error = SSUSB_DRIVER_BAD_REQUEST;
                goto error;
        }

        if (base_address == 0x00000000) {
                _driver_error = SSUSB_DRIVER_BAD_REQUEST;
                goto error;
        }

        uint8_t command;
        command = (execute) ? CMD_EXECUTE_EXT : CMD_UPLOAD;

        if ((_command_send(command, base_address, len)) < 0) {
                goto error;
        }

        if ((_device_write(buffer, len)) < 0) {
                goto error;
        }

        if ((_checksum_send(buffer, len)) < 0) {
                goto error;
        }

        goto exit;

error:
        exit_code = -1;

        /* (void)printf("ERROR: %i %s\n", __LINE__, _error_strings[_error]); */

exit:
        DEBUG_PRINTF("Exit\n");

        return exit_code;
}

static int
_command_send(protocol_command_t command, uint32_t address, size_t len)
{
#ifdef DEBUG
        static const char *command2str[] = {
                NULL,
                "CMD_DOWNLOAD",
                "CMD_UPLOAD",
                "CMD_EXECUTE",
                "CMD_GET_BUFF_ADDR",
                "CMD_COPY_EXECUTE",
                "CMD_EXECUTE_EXT"
        };
#endif /* DEBUG */

        _driver_error = SSUSB_DRIVER_OK;

        uint8_t buffer[13];
        uint8_t buffer_len;

        DEBUG_PRINTF("Command: \"%s\" (0x%02X)\n",
            command2str[command],
            command);
        DEBUG_PRINTF("Address: 0x%08X\n", address);
        DEBUG_PRINTF("Size: %iB (0x%08X)\n", (size_t)len, (size_t)len);

        buffer[ 0] = command;

        buffer[ 1] = ADDRESS_MSB(address);
        buffer[ 2] = ADDRESS_02(address);
        buffer[ 3] = ADDRESS_01(address);
        buffer[ 4] = ADDRESS_LSB(address);

        buffer[ 5] = LEN_MSB(len);
        buffer[ 6] = LEN_02(len);
        buffer[ 7] = LEN_01(len);
        buffer[ 8] = LEN_LSB(len);

        buffer_len = 9;

        if (command == CMD_EXECUTE_EXT) {
                buffer[ 9] = 0;
                buffer[10] = 0;
                buffer[11] = 0;
                buffer[12] = 0;

                buffer_len = 13;
        }

        return _device_write(buffer, buffer_len);
}

static int
_checksum_receive(const void *buffer, size_t len)
{
        _driver_error = SSUSB_DRIVER_OK;

        const crc_t checksum = crc_calculate(buffer, len);

        uint8_t read_buffer;
        if ((_device_read(&read_buffer, sizeof(read_buffer), true)) < 0) {
                return -1;
        }

        if ((crc_t)read_buffer != checksum) {
                DEBUG_PRINTF("Checksum received (0x%02X) does not match calculated (0x%02X)\n",
                    (uint8_t)read_buffer,
                    checksum);

                _driver_error = SSUSB_DRIVER_CORRUPTED_DATA;
                return -1;
        }

        return 0;
}

static int
_checksum_send(const void *buffer, size_t len)
{
        _driver_error = SSUSB_DRIVER_OK;

        const crc_t crc = crc_calculate(buffer, len);

        if ((_device_write(&crc, sizeof(crc))) < 0) {
                return -1;
        }

        uint8_t read_buffer;
        if ((_device_read(&read_buffer, sizeof(read_buffer), true)) < 0) {
                return -1;
        }

        if (read_buffer != 0) {
                _driver_error = SSUSB_DRIVER_CORRUPTED_DATA;
                return -1;
        }

        return 0;
}

const ssusb_device_driver_t __device_usb_cartridge = {
        .name            = "usb-cart",
        .description     = "USB Flash Cartridge by Anders Montonen (antime)",
        .init            = _init,
        .deinit          = _deinit,
        .error           = _error,
        .poll            = _poll,
        .fifo_alloc      = _fifo_alloc,
        .fifo_free       = _fifo_free,
        .peek            = _peek,
        .read            = _usb_cart_read,
        .send            = _usb_cart_send,
        .download_buffer = _download_buffer,
        .upload_buffer   = _upload_buffer,
        .execute_buffer  = _execute_buffer,
};