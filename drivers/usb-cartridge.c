/*
 * Copyright (c) 2012-2022 Israel Jacquez
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

#include <ftdi.h>

#include "ftdi_ext.h"
#include "crc.h"
#include "debug.h"
#include "driver.h"
#include "endianess.h"
#include "math_utilities.h"
#include "ring_buffer.h"

#define RX_TIMEOUT      5000
#define TX_TIMEOUT      1000

#define I_VENDOR         0x0403
#define I_PRODUCT        0x6001
#define I_PRODUCT_STRING "FT245R USB FIFO"
#define I_SERIAL1        "AL00P4JX"
#define I_SERIAL2        "AI05393Z"

typedef enum {
        CMD_DOWNLOAD = 1,
        CMD_UPLOAD,
        CMD_EXECUTE,
        CMD_GET_BUFFER_ADDRESS,
        CMD_COPY_EXECUTE,
        CMD_EXECUTE_EXT
} protocol_command_t;

static ssusb_driver_error_t _driver_error = SSUSB_DRIVER_OK;
static struct ftdi_context _ftdi_context;

static int _init(void);
static int _deinit(void);

static int _upload_execute_buffer(const void *buffer, uint32_t base_address,
    size_t size, bool execute);

/* Helpers */
static int _command_send(protocol_command_t command, uint32_t address, size_t size);
static int _checksum_receive(const void *buffer, size_t size);
static int _checksum_send(const void *buffer, size_t size);

#include <libusb.h>

static int
_init(void)
{
        DEBUG_PRINTF("Enter\n");

#define USB_READ_PACKET_SIZE  (64 * 1024)
#define USB_WRITE_PACKET_SIZE (4 * 1024)
#define USB_PAYLOAD(x)        ((x) - (((x) / 64) * 2))
#define READ_PAYLOAD_SIZE     (USB_PAYLOAD(USB_READ_PACKET_SIZE))
#define WRITE_PAYLOAD_SIZE    (USB_PAYLOAD(USB_WRITE_PACKET_SIZE))

        if ((ftdi_init(&_ftdi_context)) < 0) {
                DEBUG_PRINTF("ftdi_init()\n");
                return -1;
        }
        if ((ftdi_usb_open(&_ftdi_context, I_VENDOR, I_PRODUCT)) < 0) {
                DEBUG_PRINTF("ftdi_usb_open()\n");
                goto error;
        }
        if ((ftdi_tcioflush(&_ftdi_context)) < 0) {
                DEBUG_PRINTF("ftdi_tcioflush()\n");
                goto error;
        }
        if ((ftdi_read_data_set_chunksize(&_ftdi_context,
                    USB_READ_PACKET_SIZE)) < 0) {
                DEBUG_PRINTF("ftdi_read_data_set_chunksize()\n");
                goto error;
        }
        if ((ftdi_write_data_set_chunksize(&_ftdi_context,
                    USB_WRITE_PACKET_SIZE)) < 0) {
                DEBUG_PRINTF("ftdi_write_data_set_chunksize()\n");
                goto error;
        }
        if ((ftdi_set_bitmode(&_ftdi_context, 0x00, BITMODE_RESET)) < 0) {
                DEBUG_PRINTF("ftdi_set_bitmode()\n");
                goto error;
        }
        if ((ftdi_usb_match_product(&_ftdi_context, I_PRODUCT_STRING)) < 0) {
                DEBUG_PRINTF("ftdi_usb_match_product()\n");
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
_device_read(void *buffer, size_t size)
{
        return ftdi_buffered_read_data(&_ftdi_context, buffer, size);
}

static int
_device_write(const void *buffer, size_t size)
{
        return ftdi_buffered_write_data(&_ftdi_context, buffer, size);
}

static ssusb_driver_error_t
_error(void)
{
        return _driver_error;
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

static int
_download_buffer(void *buffer, uint32_t base_address, size_t size)
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

        if ((_command_send(CMD_DOWNLOAD, base_address, size)) < 0) {
                goto error;
        }

        if ((_device_read(buffer, size)) < 0) {
                goto error;
        }

        if ((_checksum_receive(buffer, size)) < 0) {
                goto error;
        }

        goto exit;

error:
        exit_code = -1;

exit:
        DEBUG_PRINTF("Exit\n");

        return exit_code;
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

        if ((_command_send(command, base_address, size)) < 0) {
                goto error;
        }

        if ((_device_write(buffer, size)) < 0) {
                goto error;
        }

        if ((_checksum_send(buffer, size)) < 0) {
                goto error;
        }

        goto exit;

error:
        exit_code = -1;

exit:
        DEBUG_PRINTF("Exit\n");

        return exit_code;
}

static int
_command_send(protocol_command_t command, uint32_t address, size_t size)
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
        uint8_t buffer_size;

        DEBUG_PRINTF("Command: \"%s\" (0x%02X)\n",
            command2str[command],
            command);
        DEBUG_PRINTF("Address: 0x%08X\n", address);
        DEBUG_PRINTF("Size: %zuB (0x%08zX)\n", size, size);

        buffer[ 0] = command;

        buffer[ 1] = ADDRESS_MSB(address);
        buffer[ 2] = ADDRESS_02(address);
        buffer[ 3] = ADDRESS_01(address);
        buffer[ 4] = ADDRESS_LSB(address);

        buffer[ 5] = SIZE_MSB(size);
        buffer[ 6] = SIZE_02(size);
        buffer[ 7] = SIZE_01(size);
        buffer[ 8] = SIZE_LSB(size);

        buffer_size = 9;

        if (command == CMD_EXECUTE_EXT) {
                buffer[ 9] = 0;
                buffer[10] = 0;
                buffer[11] = 0;
                buffer[12] = 0;

                buffer_size = 13;
        }

        return _device_write(buffer, buffer_size);
}

static int
_checksum_receive(const void *buffer, size_t size)
{
        _driver_error = SSUSB_DRIVER_OK;

        const crc_t checksum = crc_calculate(buffer, size);

        uint8_t read_buffer;
        if ((_device_read(&read_buffer, sizeof(read_buffer))) < 0) {
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
_checksum_send(const void *buffer, size_t size)
{
        _driver_error = SSUSB_DRIVER_OK;

        const crc_t crc = crc_calculate(buffer, size);

        if ((_device_write(&crc, sizeof(crc))) < 0) {
                return -1;
        }

        uint8_t read_buffer;
        if ((_device_read(&read_buffer, sizeof(read_buffer))) < 0) {
                return -1;
        }

        if (read_buffer != 0) {
                _driver_error = SSUSB_DRIVER_CORRUPTED_DATA;
                return -1;
        }

        return 0;
}

const ssusb_device_driver_t device_usb_cartridge = {
        .name            = "usb-cart",
        .description     = "USB Flash Cartridge by Anders Montonen (antime)",
        .init            = _init,
        .deinit          = _deinit,
        .error           = _error,
        .poll            = _poll,
        .peek            = _peek,
        .read            = _device_read,
        .write           = _device_write,
        .download_buffer = _download_buffer,
        .upload_buffer   = _upload_buffer,
        .execute_buffer  = _execute_buffer,
};
