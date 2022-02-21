/*
 * Copyright (c) 2012-2022 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#ifndef _LIBSSUSB_FTDI_EXT_H_
#define _LIBSSUSB_FTDI_EXT_H_

#include <ftdi.h>

#include <stddef.h>

#include <ssusb-types.h>
#include <ssusb-export.h>

DLL_LOCAL int ftdi_buffered_init(struct ftdi_context *context);
DLL_LOCAL void ftdi_buffered_deinit(struct ftdi_context *ftdi);
DLL_LOCAL int ftdi_buffered_poll(struct ftdi_context *ftdi, size_t *read_len);
DLL_LOCAL int ftdi_buffered_peek(struct ftdi_context *ftdi, size_t len, void *buffer, size_t *read_len);
DLL_LOCAL int ftdi_buffered_read_data(struct ftdi_context *ftdi, void *buffer, size_t len);
DLL_LOCAL int ftdi_buffered_write_data(struct ftdi_context *ftdi, const void *buffer, size_t len);

int ftdi_usb_get_product_string(struct ftdi_context *ftdi, char *product, size_t product_len);
int ftdi_usb_get_serial_string(struct ftdi_context *ftdi, char *serial, size_t serial_len);

int ftdi_usb_match_product(struct ftdi_context *ftdi, const char *product);

#endif /* !_LIBSSUSB_FTDI_EXT_H_ */
