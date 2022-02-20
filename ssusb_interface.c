#include <stdio.h>
#include <stdbool.h>

#include <stdlib.h>
#include <string.h>

#include "ssusb.h"

#include "file.h"

#include "drivers/driver.h"

extern ssusb_ret_t ssusb_drivers_selected_get(const ssusb_device_driver_t **device);

static ssusb_ret_t
_upload_execute_file(const char *file, uint32_t base_address, bool execute)
{
        ssusb_ret_t ret;

        const ssusb_device_driver_t *driver;
        ret = ssusb_drivers_selected_get(&driver);
        if (ret != SSUSB_OK) {
                return ret;
        }

        void *buffer;
        size_t buffer_len;
        ret = file_read(file, &buffer, &buffer_len);
        if (ret != SSUSB_OK) {
                goto exit;
        }

        int driver_ret;

        if (execute) {
                driver_ret =
                    driver->execute_buffer(buffer, base_address, buffer_len);
        } else {
                driver_ret =
                    driver->upload_buffer(buffer, base_address, buffer_len);
        }

        if (driver_ret < 0) {
                ret = (execute) ? SSUSB_DEVICE_EXECUTE_ERROR : SSUSB_DEVICE_UPLOAD_ERROR;
        }

exit:
        if (buffer == NULL) {
                free(buffer);
        }

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
                return SSUSB_DEVICE_WRITE_ERROR;
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

        void *buffer;
        if ((buffer = malloc(len)) == NULL) {
                ret = SSUSB_INSUFFICIENT_MEMORY;
                goto error;
        }
        (void)memset(buffer, 0, len);

        if ((driver->download_buffer(buffer, base_address, len)) < 0) {
                ret = SSUSB_DEVICE_DOWNLOAD_ERROR;
                goto error;
        }

        ret = file_write(output_file, buffer, len);

error:
        if (buffer == NULL) {
                free(buffer);
        }

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
