#include <stdio.h>
#include <stdbool.h>

#include <stdlib.h>
#include <string.h>

#include "ssusb.h"

#include "debug.h"
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
        size_t buffer_size;
        ret = file_read(file, &buffer, &buffer_size);
        if (ret != SSUSB_OK) {
                goto exit;
        }

        int driver_ret;

        if (execute) {
                driver_ret =
                    driver->execute_buffer(buffer, base_address, buffer_size);
        } else {
                driver_ret =
                    driver->upload_buffer(buffer, base_address, buffer_size);
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
ssusb_poll(size_t *read_size)
{
        ssusb_ret_t ret;

        const ssusb_device_driver_t *driver;
        ret = ssusb_drivers_selected_get(&driver);

        if (ret != SSUSB_OK) {
                return ret;
        }

        if ((driver->poll(read_size)) < 0) {
                return SSUSB_DEVICE_POLL_ERROR;
        }

        return SSUSB_OK;
}

ssusb_ret_t
ssusb_peek(size_t size, void *buffer, size_t *read_size)
{
        ssusb_ret_t ret;

        const ssusb_device_driver_t *driver;
        ret = ssusb_drivers_selected_get(&driver);

        if (ret != SSUSB_OK) {
                return ret;
        }

        if ((driver->peek(size, buffer, read_size)) < 0) {
                return SSUSB_DEVICE_PEEK_ERROR;
        }

        return SSUSB_OK;
}


ssusb_ret_t
ssusb_read(void *buffer, size_t size)
{
        ssusb_ret_t ret;

        const ssusb_device_driver_t *driver;
        ret = ssusb_drivers_selected_get(&driver);

        if (ret != SSUSB_OK) {
                return ret;
        }

        if ((driver->read(buffer, size)) < 0) {
                return SSUSB_DEVICE_READ_ERROR;
        }

        return SSUSB_OK;
}

ssusb_ret_t
ssusb_write(const void *buffer, size_t size)
{
        ssusb_ret_t ret;

        const ssusb_device_driver_t *driver;
        ret = ssusb_drivers_selected_get(&driver);

        if (ret != SSUSB_OK) {
                return ret;
        }

        if ((driver->write(buffer, size)) < 0) {
                return SSUSB_DEVICE_WRITE_ERROR;
        }

        return SSUSB_OK;
}

ssusb_ret_t
ssusb_file_download(const char *output_file, uint32_t base_address, size_t size)
{
        ssusb_ret_t ret;

        const ssusb_device_driver_t *driver;
        ret = ssusb_drivers_selected_get(&driver);

        if (ret != SSUSB_OK) {
                return ret;
        }

        void *buffer;
        if ((buffer = malloc(size)) == NULL) {
                ret = SSUSB_INSUFFICIENT_MEMORY;
                goto error;
        }
        (void)memset(buffer, 0x00, size);

        if ((driver->download_buffer(buffer, base_address, size)) < 0) {
                ret = SSUSB_DEVICE_DOWNLOAD_ERROR;
                goto error;
        }

        ret = file_write(output_file, buffer, size);

error:
        if (buffer == NULL) {
                free(buffer);
        }

        return ret;
}

ssusb_ret_t
ssusb_file_upload(const char *input_file, uint32_t base_address)
{
        return _upload_execute_file(input_file, base_address,
            /* execute = */ false);
}

ssusb_ret_t
ssusb_file_execute(const char *input_file, uint32_t base_address)
{
        return _upload_execute_file(input_file, base_address,
            /* execute = */ true);
}
