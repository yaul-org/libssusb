/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#ifndef LIBSSUSB_SHARED_H_
#define LIBSSUSB_SHARED_H_

#include <inttypes.h>

/* Compatibility with Linux and FreeBSD */
#ifndef __unused
#define __unused __attribute__ ((unused))
#endif /* !__unused */

#define ADDRESS_MSB(x)  ((uint8_t)((x) >> 24) & 0xFF)
#define ADDRESS_02(x)   ((uint8_t)((x) >> 16) & 0xFF)
#define ADDRESS_01(x)   ((uint8_t)((x) >> 8) & 0xFF)
#define ADDRESS_LSB(x)  ((uint8_t)(x) & 0xFF)

#define LEN_MSB(x)      (ADDRESS_MSB(x))
#define LEN_02(x)       (ADDRESS_02(x))
#define LEN_01(x)       (ADDRESS_01(x))
#define LEN_LSB(x)      (ADDRESS_LSB(x))

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define TO_LE(x) (__builtin_bswap32((x)))
#else
#define TO_LE(x) (x)
#endif

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define TO_BE(x) (__builtin_bswap32((x)))
#else
#define TO_BE(x) (x)
#endif

#ifdef HAVE_LIBFTD2XX
#define MAX_ENUMERATE_DEVICES 16

extern FT_HANDLE ft_handle;
extern FT_STATUS ft_error;
extern const char *ft_error_strings[];

const char **enumerate_devices(void);
#else
extern struct ftdi_context ftdi_ctx;
extern int ftdi_error;
#endif /* HAVE_LIBFTD2XX */

#ifndef min
#define min(a, b)                                                              \
        __extension__ ({ __typeof__ (a) _a = (a);                              \
           __typeof__ (b) _b = (b);                                            \
           (_a < _b) ? _a : _b;                                                \
        })
#endif /* !min */

#ifndef max
#define max(a, b)                                                              \
        __extension__ ({ __typeof__ (a) _a = (a);                              \
           __typeof__ (b) _b = (b);                                            \
           (_a > _b) ? _a : _b;                                                \
        })
#endif /* !max */

int verbose_printf(const char *, ...);

#ifdef DEBUG
void debug_hexdump(const uint8_t *, uint32_t);
#endif /* DEBUG */

#endif /* !LIBSSUSB_SHARED_H_ */
