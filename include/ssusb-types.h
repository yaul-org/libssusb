/*
 * Copyright (c) 2012-2022 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#ifndef LIBSSUSB_SSUSB_TYPES_H
#define LIBSSUSB_SSUSB_TYPES_H

typedef struct ssusb_driver ssusb_driver_t;

struct ssusb_driver {
        const char *name;
        const char *description;

        ssusb_driver_t *next;
};

typedef enum {
        SSUSB_OK,

        SSUSB_BAD_ARG,
        SSUSB_INSUFFICIENT_MEMORY,

        SSUSB_INIT_ERROR,

        SSUSB_SELECT_INVALID_NAME,
        SSUSB_SELECT_NOT_FOUND,
        SSUSB_SELECT_INIT_ERROR,

        SSUSB_DESELECT_DEINIT_ERROR,

        SSUSB_DEVICE_POLL_ERROR,
        SSUSB_DEVICE_PEEK_ERROR,
        SSUSB_DEVICE_READ_ERROR,
        SSUSB_DEVICE_WRITE_ERROR,
        SSUSB_DEVICE_DOWNLOAD_ERROR,
        SSUSB_DEVICE_UPLOAD_ERROR,
        SSUSB_DEVICE_EXECUTE_ERROR,

        SSUSB_FILE_INVALID_PATH,
        SSUSB_FILE_NOT_EXIST,
        SSUSB_FILE_NOT_FILE,
        SSUSB_FILE_PERMISSION_ACCESS,
        SSUSB_FILE_EMPTY,
        SSUSB_FILE_UNKNOWN_ERROR,
} ssusb_ret_t;

#endif /* !LIBSSUSB_SSUSB_TYPES_H */
