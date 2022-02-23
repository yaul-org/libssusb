/*
 * Copyright (c) 2012-2022 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <stdio.h>
#include <string.h>

#include "debug.h"
#include <math_utilities.h>

void
debug_hexdump(const char *buffer, size_t size, uint32_t max_width)
{
        if (size == 0) {
                return;
        }

        max_width = min(min(size, max_width), 80U);

        (void)fputs("  ", stderr);

        for (uint32_t i = 0; i < max_width; i++) {
                (void)fprintf(stderr, " [1;35m%02X[m", i);
        }

        uint32_t fold;
        fold = 0;

        for (uint32_t i = 0; i < size; i++) {
                const bool eol = ((fold % max_width) == 0);
                const bool last_byte = ((i + 1) == size);

                if (eol && !last_byte) {
                        (void)fprintf(stderr, "\n[1;35m%02X[m %02X", i, buffer[i]);
                } else {
                        (void)fprintf(stderr, " %02X", buffer[i]);
                }

                fold++;
        }

        (void)putc('\n', stderr);

        (void)fflush(stderr);
}
