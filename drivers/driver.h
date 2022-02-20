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

#define SSUSB_DRIVER_NAME_LEN        (32)
#define SSUSB_DRIVER_DESCRIPTION_LEN (256)

typedef enum {
        SSUSB_DRIVER_OK,
        SSUSB_DRIVER_DEVICE_ERROR,
        SSUSB_DRIVER_IO_ERROR,
        SSUSB_DRIVER_INSUFFICIENT_READ_DATA,
        SSUSB_DRIVER_INSUFFICIENT_WRITE_DATA,
        SSUSB_DRIVER_CORRUPTED_DATA,
        SSUSB_DRIVER_BAD_REQUEST,
        SSUSB_DRIVER_INSUFFICIENT_MEMORY,
        SSUSB_DRIVER_PARAM_NULL,
} ssusb_driver_error_t;

typedef struct {
        const char name[SSUSB_DRIVER_NAME_LEN];
        const char description[SSUSB_DRIVER_DESCRIPTION_LEN];

        int (*init)(void);
        int (*deinit)(void);

        ssusb_driver_error_t (*error)(void);

        int (*poll)(size_t *read_len);
        int (*peek)(size_t len, void *buffer, size_t *read_len);
        int (*read)(void *buffer, size_t len);
        int (*write)(const void *buffer, size_t len);

        int (*download_buffer)(void *buffer, uint32_t base_address, size_t len);
        int (*upload_buffer)(const void *buffer, uint32_t base_address, size_t len);
        int (*execute_buffer)(const void *buffer, uint32_t base_address, size_t len);
} ssusb_device_driver_t;

#endif /* !_LIBSSUSB_DRIVER_H_ */
