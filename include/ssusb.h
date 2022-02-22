/*
 * Copyright (c) 2012-2022 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#ifndef LIBSSUSB_SSUSB_H
#define LIBSSUSB_SSUSB_H

#include <stddef.h>
#include <inttypes.h>

#include <ssusb-types.h>
#include <ssusb-export.h>

DLL_PUBLIC ssusb_ret_t ssusb_init(void);
DLL_PUBLIC void ssusb_deinit(void);

DLL_PUBLIC ssusb_ret_t ssusb_drivers_select(const char *driver_name);
DLL_PUBLIC ssusb_ret_t ssusb_drivers_deselect(void);

DLL_PUBLIC ssusb_ret_t ssusb_drivers_list_get(const ssusb_driver_t **driver_list);

DLL_PUBLIC ssusb_ret_t ssusb_read(void *buffer, size_t size);
DLL_PUBLIC ssusb_ret_t ssusb_write(const void *buffer, size_t size);
DLL_PUBLIC ssusb_ret_t ssusb_download_file(const char *output_file, uint32_t base_address, size_t size);
DLL_PUBLIC ssusb_ret_t ssusb_upload_file(const char *input_file, uint32_t base_address);
DLL_PUBLIC ssusb_ret_t ssusb_execute_file(const char *input_file, uint32_t base_address);

#endif /* !LIBSSUSB_SSUSB_H */
