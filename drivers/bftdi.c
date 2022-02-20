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

#include "bftdi.h"
#include "cdefs.h"
#include "debug.h"
#include "math_utilities.h"
#include "ring_buffer.h"

#define RING_BUFFER_SIZE (64 * 1024)

struct bftdi {
        struct ftdi_context *context;
        ring_buffer_t rb;
};

static int _device_read(bftdi_t bftdi, void *buffer, size_t len, bool block, size_t *read_len);
static int _device_write(bftdi_t bftdi, const void *buffer, size_t len, size_t *write_len);

bftdi_t
bftdi_create(struct ftdi_context *context)
{
        struct bftdi * const bftdi = malloc(sizeof(struct bftdi));
        if (bftdi == NULL) {
                return NULL;
        }

        bftdi->context = context;

        if ((ring_buffer_init(&bftdi->rb, RING_BUFFER_SIZE)) < 0) {
                free(bftdi);

                return NULL;
        }

        return bftdi;
}

void
bftdi_destroy(bftdi_t bftdi)
{
        if (bftdi == NULL) {
                return;
        }

        free(bftdi);
}

int
bftdi_poll(bftdi_t bftdi, size_t *read_len)
{
        struct bftdi *_bftdi = bftdi;

        uint8_t byte;

        if ((ftdi_read_data(_bftdi->context, &byte, sizeof(byte))) < 0) {
                /* _ftdi_error = SSUSB_DRIVER_DEVICE_ERROR; */
                return -1;
        }

        ring_buffer_queue(&_bftdi->rb, byte);

        if (read_len != NULL) {
                *read_len = ring_buffer_size(&_bftdi->rb);
        }

        return 0;
}

int
bftdi_peek(bftdi_t bftdi, size_t len, void *buffer, size_t *read_len)
{
        /* _driver_error = SSUSB_DRIVER_OK; */

        assert(buffer != NULL);
        assert(read_len != NULL);

        struct bftdi *_bftdi = bftdi;

        const ring_buffer_size_t rb_size = ring_buffer_size(&_bftdi->rb);

        *read_len = min(rb_size, len);

        uint8_t * buffer_pos = buffer;
        for (size_t i = 0; i < *read_len; i++, buffer_pos++) {
                ring_buffer_peek(&_bftdi->rb, buffer_pos, (rb_size - 1) - i);
        }

        return 0;
}

int
bftdi_read(bftdi_t bftdi, void *buffer, size_t len)
{
        assert(buffer != NULL);

        if ((_device_read(bftdi, buffer, len, true, NULL)) < 0) {
                return -1;
        }

        return 0;
}

int
bftdi_write(bftdi_t bftdi, const void *buffer, size_t len)
{
        assert(buffer != NULL);

        if ((_device_write(bftdi, buffer, len, NULL)) < 0) {
                return -1;
        }

        return 0;
}

static int
_device_read(bftdi_t bftdi, void *buffer, size_t len, bool block, size_t *read_len)
{
        struct bftdi *_bftdi = bftdi;

        /* _driver_error = SSUSB_DRIVER_OK; */

        uint8_t *buffer_pos;
        buffer_pos = buffer;

        const ring_buffer_size_t rb_size = ring_buffer_size(&_bftdi->rb);
        const uint32_t preread_amount = min(rb_size, len);

        ring_buffer_array_dequeue(&_bftdi->rb, buffer_pos, preread_amount);

        buffer_pos += preread_amount;

        if (read_len != NULL) {
                *read_len = 0;
        }

        while (((uintptr_t)buffer_pos - (uintptr_t)buffer) < len) {
                int read;
                if ((read = ftdi_read_data(_bftdi->context, buffer_pos, len)) < 0) {
                        /* _driver_error = */
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
_device_write(bftdi_t bftdi, const void *buffer, size_t len, size_t *write_len)
{
        struct bftdi *_bftdi = bftdi;

        /* _driver_error = SSUSB_DRIVER_OK; */

        uint32_t written;
        written = 0;

        if (write_len != NULL) {
                *write_len = 0;
        }

        while ((len - written) > 0) {
                int write;
                if ((write = ftdi_write_data(_bftdi->context, buffer, len)) < 0) {
                        return -1;
                }

                written += write;

                if (write_len != NULL) {
                        *write_len += write;
                }
        }

        return 0;
}
