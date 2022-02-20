/*
 * Copyright (c) 2012-2022 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <stdio.h>
#include <string.h>

#include "debug.h"

void
debug_hexdump(const uint8_t *buffer, uint32_t len)
{
        if (len == 0) {
                return;
        }

        static char output_buffer[2048];

        uint32_t output_idx;
        output_idx = 0;

        uint32_t fold_x;
        fold_x = 0;

        /* Skip 1st line */
        (void)sprintf(output_buffer, "\nHH HH HH HH HH HH HH HH\n");
        output_idx += strlen(output_buffer);

        uint32_t buffer_idx;
        for (buffer_idx = 0; buffer_idx < len; buffer_idx++) {
                (void)sprintf(&output_buffer[output_idx],
                    "%02X", buffer[buffer_idx]);

                output_idx += 2;

                if (((fold_x + 1) % 20) == 0) {
                        output_buffer[output_idx] = '\n';
                        output_idx++;
                } else if ((buffer_idx + 1) != len) {
                        output_buffer[output_idx] = ' ';
                        output_idx++;
                }

                fold_x++;
        }

        if (output_buffer[output_idx] != '\n') {
                output_buffer[output_idx] = '\n';
                output_idx++;
        }

        output_buffer[output_idx] = '\0';

        DEBUG_PRINTF("%s", output_buffer);
}
