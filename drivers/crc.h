/*
 * Copyright (c) 2012-2022 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#ifndef _LIBSSUSB_CRC_H_
#define _LIBSSUSB_CRC_H_

#include <inttypes.h>
#include <stdlib.h>

typedef uint8_t crc_t;

crc_t crc_calculate(const uint8_t *buffer, size_t buffer_len);

#endif /* !_LIBSSUSB_CRC_H_ */
