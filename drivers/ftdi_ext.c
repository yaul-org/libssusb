/*
 * Copyright (c) 2012-2022 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <libusb.h>

#include "ftdi_ext.h"
#include "cdefs.h"
#include "debug.h"
#include "math_utilities.h"
#include "ring_buffer.h"

#define RING_BUFFER_SIZE (64 * 1024)

static uint8_t _buffer[RING_BUFFER_SIZE];

static ring_buffer_t _ring_buffer = {
        .buffer      = _buffer,
        .buffer_size = RING_BUFFER_SIZE,
        .tail_index  = 0,
        .head_index  = 0
};

static int _device_read(struct ftdi_context *ftdi, void *buffer, size_t len, bool block, size_t *read_len);
static int _device_write(struct ftdi_context *ftdi, const void *buffer, size_t len, size_t *write_len);

int
ftdi_usb_get_product_string(struct ftdi_context *ftdi, char *product, size_t product_len)
{
        struct libusb_device * const usb_dev = libusb_get_device(ftdi->usb_dev);

        return ftdi_usb_get_strings2(ftdi,
            usb_dev,
            NULL,
            0,
            product,
            product_len,
            NULL,
            0);
}

int
ftdi_usb_get_serial_string(struct ftdi_context *ftdi, char *serial, size_t serial_len)
{
        struct libusb_device * const usb_dev = libusb_get_device(ftdi->usb_dev);

        return ftdi_usb_get_strings2(ftdi,
            usb_dev,
            NULL,
            0,
            NULL,
            0,
            serial,
            serial_len);
}

int
ftdi_usb_match_product(struct ftdi_context *ftdi, const char *product)
{
        char buffer[256];

        int ret = ftdi_usb_get_product_string(ftdi, buffer, sizeof(buffer));
        if (ret < 0) {
                return ret;
        }

        if ((strncmp(buffer, product, strlen(product))) != 0) {
                return -14; /* No match */
        }

        return 0;
}

int
ftdi_buffered_poll(struct ftdi_context *ftdi, size_t *read_len)
{
        uint8_t byte;

        if ((ftdi_read_data(ftdi, &byte, sizeof(byte))) < 0) {
                DEBUG_PRINTF("ftdi_read_data: %s\n", ftdi_get_error_string(ftdi));
                return -1;
        }

        ring_buffer_queue(&_ring_buffer, byte);

        if (read_len != NULL) {
                *read_len = ring_buffer_size(&_ring_buffer);
        }

        return 0;
}

int
ftdi_buffered_peek(struct ftdi_context *ftdi __unused, size_t len, void *buffer, size_t *read_len)
{
        assert(buffer != NULL);
        assert(read_len != NULL);

        const ring_buffer_size_t rb_size = ring_buffer_size(&_ring_buffer);

        *read_len = min(rb_size, len);

        uint8_t * buffer_pos = buffer;
        for (size_t i = 0; i < *read_len; i++, buffer_pos++) {
                ring_buffer_peek(&_ring_buffer, buffer_pos, (rb_size - 1) - i);
        }

        return 0;
}

int
ftdi_buffered_read_data(struct ftdi_context *ftdi, void *buffer, size_t len)
{
        assert(buffer != NULL);

        if ((_device_read(ftdi, buffer, len, true, NULL)) < 0) {
                return -1;
        }

        return 0;
}

int
ftdi_buffered_write_data(struct ftdi_context *ftdi, const void *buffer, size_t len)
{
        assert(buffer != NULL);

        if ((_device_write(ftdi, buffer, len, NULL)) < 0) {
                return -1;
        }

        return 0;
}

static int
_device_read(struct ftdi_context *ftdi, void *buffer, size_t len, bool block, size_t *read_len)
{
        uint8_t *buffer_pos;
        buffer_pos = buffer;

        const ring_buffer_size_t rb_size = ring_buffer_size(&_ring_buffer);
        const uint32_t preread_amount = min(rb_size, len);

        ring_buffer_array_dequeue(&_ring_buffer, buffer_pos, preread_amount);

        buffer_pos += preread_amount;

        if (read_len != NULL) {
                *read_len = 0;
        }

        while (((uintptr_t)buffer_pos - (uintptr_t)buffer) < len) {
                int read;
                if ((read = ftdi_read_data(ftdi, buffer_pos, len)) < 0) {
                        DEBUG_PRINTF("ftdi_read_data: %s\n", ftdi_get_error_string(ftdi));
                        return -1;
                }

                buffer_pos += read;

                if (read_len != NULL) {
                        *read_len += read;
                }

                if (!block) {
                        return 0;
                }
        }

        return 0;
}

static int
_device_write(struct ftdi_context *ftdi, const void *buffer, size_t len, size_t *write_len)
{
        uint32_t written;
        written = 0;

        if (write_len != NULL) {
                *write_len = 0;
        }

        while ((len - written) > 0) {
                int write;
                if ((write = ftdi_write_data(ftdi, buffer, len)) < 0) {
                        DEBUG_PRINTF("ftdi_write_data: %s\n", ftdi_get_error_string(ftdi));
                        return -1;
                }

                written += write;

                if (write_len != NULL) {
                        *write_len += write;
                }
        }

        return 0;
}
