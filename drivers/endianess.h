/*
 * Copyright (c) 2012-2022 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#ifndef _LIBSSUSB_ENDIANESS_H_
#define _LIBSSUSB_ENDIANESS_H_

#include <inttypes.h>

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

#endif /* !_LIBSSUSB_ENDIANESS_H_ */
