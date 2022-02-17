/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#ifndef LIBSSUSB_FIFO_H_
#define LIBSSUSB_FIFO_H_

#include <stddef.h>
#include <inttypes.h>

struct fifo {
        const uint8_t *buffer;
        size_t size;
};

#endif /* !LIBSSUSB_FIFO_H_ */
