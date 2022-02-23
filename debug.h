/*
 * Copyright (c) 2012-2022 Israel Jacquez
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
        (void)fprintf(stderr, "%s():%s:L%i:" " " fmt,                          \
            __func__, __FILE__, __LINE__,                                      \
            ##__VA_ARGS__);                                                    \
        (void)fflush(stderr);                                                  \
} while(false)
#else
#define DEBUG_PRINTF(...)
#endif /* DEBUG */

#ifdef DEBUG
#include <inttypes.h>

void debug_hexdump(const char *buffer, size_t size, uint32_t width);

#define DEBUG_HEXDUMP(buffer, size) do {                                       \
        debug_hexdump((void *)buffer, size, 24);                               \
} while (false)
#else
#define DEBUG_HEXDUMP(...)
#endif /* DEBUG */

#endif /* !_LIBSSUSB_DEBUG_H_ */
