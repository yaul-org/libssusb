/*
 * Copyright (c) 2012-2022 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#ifndef _LIBSSUSB_FILE_H_
#define _LIBSSUSB_FILE_H_

#include <stdio.h>

#include <ssusb-types.h>
#include <ssusb-export.h>

DLL_LOCAL ssusb_ret_t file_read(const char *input_file, void **buffer, size_t *len);
DLL_LOCAL ssusb_ret_t file_write(const char *output_file, const void *buffer, size_t len);

#endif /* !_LIBSSUSB_FILE_H_ */
