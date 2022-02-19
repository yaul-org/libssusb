#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "file.h"

void
__file_init(file_io_t *file)
{
        file->stream = NULL;
        file->buffer = NULL;
        file->len = 0;
}

ssusb_ret_t
__file_open(const char *input_file, file_io_t *file)
{
        if ((input_file == NULL) || (*input_file == '\0')) {
                return SSUSB_FILE_INVALID_PATH;
        }

        if ((file->stream = fopen(input_file, "rb+")) == NULL) {
                return SSUSB_FILE_OPEN_ERROR;
        }

        return SSUSB_OK;
}

ssusb_ret_t
__file_create(const char *output_file, file_io_t *file)
{
        if ((output_file == NULL) || (*output_file == '\0')) {
                return SSUSB_FILE_INVALID_PATH;
        }

        ssusb_ret_t ret;
        ret = SSUSB_OK;

        if ((file->stream = fopen(output_file, "wb+")) == NULL) {
                ret = SSUSB_FILE_IO_ERROR;
        }

        return ret;
}

void
__file_close(file_io_t *file)
{
        if (file == NULL) {
                return;
        }

        if (file->stream != NULL) {
                fclose(file->stream);
                file->stream = NULL;
        }

        if (file->buffer != NULL) {
                free(file->buffer);
                file->buffer = NULL;
        }

        file->len = 0;
}

ssusb_ret_t
__file_read(file_io_t *file)
{
        ssusb_ret_t ret;
        ret = SSUSB_OK;

        /* Determine the size of file */
        int32_t len;
        if ((fseek(file->stream, 0, SEEK_END)) < 0) {
                ret = SSUSB_FILE_IO_ERROR;
                goto error;
        }
        long tell;
        if ((tell = ftell(file->stream)) < 0) {
                ret = SSUSB_FILE_IO_ERROR;
                goto error;
        }
        rewind(file->stream);

        /* XXX: Pad out the file? */
        if ((len = (int32_t)tell) < 2) {
                ret = SSUSB_FILE_IO_ERROR;
                /* _driver_error = SSUSB_DRIVER_BAD_REQUEST; */
                goto error;
        }

        uint8_t *buffer;
        if ((buffer = (uint8_t *)malloc(len)) == NULL) {
                ret = SSUSB_INSUFFICIENT_MEMORY;
                goto error;
        }
        (void)memset(buffer, 0, len);

        size_t read;
        if ((read = fread(buffer, 1, len, file->stream)) != (size_t)len) {
                ret = SSUSB_FILE_IO_ERROR;
                goto error;
        }

        file->buffer = buffer;
        file->len = len;

        (void)fclose(file->stream);
        file->stream = NULL;

        return ret;

error:
        __file_close(file);

        return ret;
}

ssusb_ret_t
__file_write(file_io_t *file)
{
        if ((fwrite(file->buffer, 1, file->len, file->stream)) != file->len) {
                __file_close(file);
                return SSUSB_FILE_IO_ERROR;
        }

        return SSUSB_OK;
}
