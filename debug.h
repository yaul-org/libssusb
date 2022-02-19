/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#ifndef _LIBSSUSB_DEBUG_H_
#define _LIBSSUSB_DEBUG_H_

#ifdef DEBUG
#include <stdio.h>
#include <stdbool.h>

#define DEBUG_PRINTF(fmt, ...) do {                                            \
        (void)fprintf(stderr, "%s():L%i:" " " fmt, __FUNCTION__, __LINE__,     \
            ##__VA_ARGS__);                                                    \
        (void)fflush(stderr);                                                  \
} while(false)
#else
#define DEBUG_PRINTF(x...)
#endif /* DEBUG */

#ifdef DEBUG
#include <inttypes.h>

void __debug_hexdump(const uint8_t *, uint32_t);

#define DEBUG_HEXDUMP(buffer, len) do {                                        \
        __debug_hexdump(buffer, len);                                          \
} while (false)
#else
#define DEBUG_HEXDUMP(...)
#endif /* DEBUG */

#endif /* !_LIBSSUSB_DEBUG_H_ */
