/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#ifndef _LIBSSUSB_ILE_H_
#define _LIBSSUSB_ILE_H_

#include <stdio.h>

#include <ssusb-types.h>

typedef struct {
        FILE *stream;
        void *buffer;
        size_t len;
} file_io_t;

void __file_init(file_io_t *file);
ssusb_ret_t __file_open(const char *input_file, file_io_t *file);
ssusb_ret_t __file_create(const char *output_file, file_io_t *file);
void __file_close(file_io_t *file);
ssusb_ret_t __file_read(file_io_t *file);
ssusb_ret_t __file_write(file_io_t *file);

#endif /* !_LIBSSUSB_ILE_H_ */
