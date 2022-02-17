/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#ifndef LIBSSUSB_DRIVER_H_
#define LIBSSUSB_DRIVER_H_

#include <inttypes.h>

struct fifo;

struct device_driver {
        const char *name;

        int (*init)(void);
        int (*shutdown)(void);
        const char *(*error_stringify)(void);

        int (*test_rx_fifo)(void);
        const struct fifo *(*peek_rx_fifo)(void);

        int (*fast_read)(void *buffer, uint32_t len);

        int (*read)(void *buffer, uint32_t len);
        int (*send)(void *buffer, uint32_t len);
        int (*download_buffer)(void *buffer, uint32_t base_address, uint32_t len);
        int (*download_file)(const char *output_file, uint32_t base_address, uint32_t len);
        int (*upload_buffer)(void *buffer, uint32_t base_address, uint32_t len);
        int (*upload_file)(const char *input_file, uint32_t base_address);
        int (*execute_buffer)(void *buffer, uint32_t base_address, uint32_t len);
        int (*execute_file)(const char *input_file, uint32_t base_address);
};

#endif /* !LIBSSUSB_DRIVER_H_ */
