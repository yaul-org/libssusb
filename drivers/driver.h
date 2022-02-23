/*
 * Copyright (c) 2012-2022 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#ifndef _LIBSSUSB_DRIVER_H_
#define _LIBSSUSB_DRIVER_H_

#include <stddef.h>
#include <stdio.h>
#include <inttypes.h>

#define SSUSB_DRIVER_NAME_SIZE        (32)
#define SSUSB_DRIVER_DESCRIPTION_SIZE (256)

typedef enum {
        SSUSB_DRIVER_OK,
        SSUSB_DRIVER_DEVICE_ERROR,
        SSUSB_DRIVER_IO_ERROR,
        SSUSB_DRIVER_INSUFFICIENT_READ_DATA,
        SSUSB_DRIVER_INSUFFICIENT_WRITE_DATA,
        SSUSB_DRIVER_CORRUPTED_DATA,
        SSUSB_DRIVER_BAD_REQUEST,
        SSUSB_DRIVER_INSUFFICIENT_MEMORY,
} ssusb_driver_error_t;

typedef struct {
        const char name[SSUSB_DRIVER_NAME_SIZE];
        const char description[SSUSB_DRIVER_DESCRIPTION_SIZE];

        int (*init)(void);
        int (*deinit)(void);

        ssusb_driver_error_t (*error)(void);

        int (*poll)(size_t *read_size);
        int (*peek)(size_t size, void *buffer, size_t *read_size);
        int (*read)(void *buffer, size_t size);
        int (*write)(const void *buffer, size_t size);

        int (*download_buffer)(void *buffer, uint32_t base_address, size_t size);
        int (*upload_buffer)(const void *buffer, uint32_t base_address, size_t size);
        int (*execute_buffer)(const void *buffer, uint32_t base_address, size_t size);
} ssusb_device_driver_t;

#endif /* !_LIBSSUSB_DRIVER_H_ */
