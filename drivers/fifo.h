/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#ifndef _LIBSSUSB_FIFO_H_
#define _LIBSSUSB_FIFO_H_

#include <inttypes.h>
#include <stddef.h>

struct fifo {
        const uint8_t *buffer;
        size_t size;
};

#endif /* !_LIBSSUSB_FIFO_H_ */
