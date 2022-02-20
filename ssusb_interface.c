#include <stdio.h>
#include <stdbool.h>

#include <stdlib.h>
#include <string.h>

#include "ssusb.h"

#include "file.h"

#include "drivers/driver.h"

extern ssusb_ret_t ssusb_drivers_selected_get(const ssusb_device_driver_t **device);

static ssusb_ret_t
_upload_execute_file(const char *input_file, uint32_t base_address,
    bool execute)
{
        ssusb_ret_t ret;

        const ssusb_device_driver_t *driver;
        ret = ssusb_drivers_selected_get(&driver);
        if (ret != SSUSB_OK) {
                return ret;
        }

        file_io_t file;
        file_init(&file);

        ret = file_open(input_file, &file);
        if (ret != SSUSB_OK) {
                return ret;
        }

        ret = file_read(&file);
        if (ret != SSUSB_OK) {
                return ret;
        }

        int driver_ret;

        if (execute) {
                driver_ret = driver->execute_buffer(file.buffer,
                    base_address, file.len);
        } else {
                driver_ret = driver->upload_buffer(file.buffer,
                    base_address, file.len);
        }

        if (driver_ret < 0) {
                ret = (execute) ? SSUSB_DEVICE_EXECUTE_ERROR : SSUSB_DEVICE_UPLOAD_ERROR;
        }

        file_close(&file);

        return ret;
}

ssusb_ret_t
ssusb_read(void *buffer, size_t len)
{
        ssusb_ret_t ret;

        const ssusb_device_driver_t *driver;
        ret = ssusb_drivers_selected_get(&driver);

        if (ret != SSUSB_OK) {
                return ret;
        }

        if ((driver->read(buffer, len)) < 0) {
                return SSUSB_DEVICE_READ_ERROR;
        }

        return SSUSB_OK;
}

ssusb_ret_t
ssusb_write(const void *buffer, size_t len)
{
        ssusb_ret_t ret;

        const ssusb_device_driver_t *driver;
        ret = ssusb_drivers_selected_get(&driver);

        if (ret != SSUSB_OK) {
                return ret;
        }

        if ((driver->write(buffer, len)) < 0) {
                return SSUSB_DEVICE_READ_ERROR;
        }

        return SSUSB_OK;
}

ssusb_ret_t
ssusb_download_file(const char *output_file, uint32_t base_address, size_t len)
{
        ssusb_ret_t ret;

        const ssusb_device_driver_t *driver;
        ret = ssusb_drivers_selected_get(&driver);

        if (ret != SSUSB_OK) {
                return ret;
        }

        file_io_t file;
        file_init(&file);

        ret = file_create(output_file, &file);
        if (ret != SSUSB_OK) {
                return ret;
        }

        if ((file.buffer = malloc(file.len)) == NULL) {
                ret = SSUSB_INSUFFICIENT_MEMORY;
                goto error;
        }
        (void)memset(file.buffer, 0, file.len);

        file.len = len;

        if ((driver->download_buffer(file.buffer, base_address, file.len)) < 0) {
                ret = SSUSB_DEVICE_DOWNLOAD_ERROR;
                goto error;
        }

        file_write(&file);

error:
        file_close(&file);

        return ret;
}

ssusb_ret_t
ssusb_upload_file(const char *input_file, uint32_t base_address)
{
        return _upload_execute_file(input_file, base_address,
            /* execute = */ false);
}

ssusb_ret_t
ssusb_execute_file(const char *input_file, uint32_t base_address)
{
        return _upload_execute_file(input_file, base_address,
            /* execute = */ true);
}
