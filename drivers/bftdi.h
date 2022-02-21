/*
 * Copyright (c) 2012-2022 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#ifndef _LIBSSUSB_BFTDI_H_
#define _LIBSSUSB_BFTDI_H_

#include <ftdi.h>

#include <stddef.h>

#include <ssusb-types.h>
#include <ssusb-export.h>

typedef void *bftdi_t;

DLL_LOCAL bftdi_t bftdi_create(struct ftdi_context *context);
DLL_LOCAL void bftdi_destroy(bftdi_t bftdi);
DLL_LOCAL int bftdi_poll(bftdi_t bftdi, size_t *read_len);
DLL_LOCAL int bftdi_peek(bftdi_t bftdi, size_t len, void *buffer, size_t *read_len);
DLL_LOCAL int bftdi_read(bftdi_t bftdi, void *buffer, size_t len);
DLL_LOCAL int bftdi_write(bftdi_t bftdi, const void *buffer, size_t len);

int ftdi_usb_get_product_string(struct ftdi_context *ftdi, char *product, size_t product_len);
int ftdi_usb_get_serial_string(struct ftdi_context *ftdi, char *serial, size_t serial_len);

int ftdi_usb_match_product(struct ftdi_context *ftdi, const char *product);

#endif /* !_LIBSSUSB_BFTDI_H_ */
