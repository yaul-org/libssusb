#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <sys/stat.h>

#include "file.h"
#include "math_utilities.h"

static ssusb_ret_t
_errno_convert(void)
{
        switch (errno) {
        case EXIT_SUCCESS:
                return SSUSB_OK;
        case EACCES:
                return SSUSB_FILE_PERMISSION_ACCESS;
        case ENAMETOOLONG:
        case ENOENT:
                return SSUSB_FILE_INVALID_PATH;
        default:
                return SSUSB_FILE_UNKNOWN_ERROR;
        }
}

void
file_init(file_io_t *file)
{
        file->stream = NULL;
        file->buffer = NULL;
        file->len = 0;
}

ssusb_ret_t
file_exists(const char *input_file)
{
        if ((input_file == NULL) || (*input_file == '\0')) {
                return SSUSB_FILE_INVALID_PATH;
        }

        /* Determine if the file exists */
        struct stat stat_buffer;
        if ((stat(input_file, &stat_buffer)) != 0) {
                return SSUSB_FILE_NOT_EXIST;
        }

        /* Determine if the path is a file */
        if ((stat_buffer.st_mode & S_IFMT) != S_IFREG) {
                return SSUSB_FILE_NOT_FILE;
        }

        return SSUSB_OK;
}

ssusb_ret_t
file_open(const char *input_file, file_io_t *file)
{
        ssusb_ret_t ret;
        ret = file_exists(input_file);
        if (ret != SSUSB_OK) {
                return ret;
        }

        if ((file->stream = fopen(input_file, "rb+")) == NULL) {
                return _errno_convert();
        }

        return SSUSB_OK;
}

ssusb_ret_t
file_create(const char *output_file, file_io_t *file)
{
        ssusb_ret_t ret;
        ret = file_exists(output_file);
        if (ret != SSUSB_OK) {
                return ret;
        }

        if ((file->stream = fopen(output_file, "wb+")) == NULL) {
                return _errno_convert();
        }

        return ret;
}

void
file_close(file_io_t *file)
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
file_read(file_io_t *file)
{
        ssusb_ret_t ret;
        ret = SSUSB_OK;

        int32_t len;

        /* Determine the size of file */
        if ((fseek(file->stream, 0, SEEK_END)) < 0) {
                ret = _errno_convert();
                goto error;
        }
        if ((len = ftell(file->stream)) < 0) {
                ret = _errno_convert();
                goto error;
        }
        rewind(file->stream);

        if (len == 0) {
                ret = SSUSB_FILE_EMPTY;
                goto error;
        }

        uint8_t *buffer;
        if ((buffer = malloc(len)) == NULL) {
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

        return ret;

error:
        file_close(file);

        return ret;
}

ssusb_ret_t
file_write(file_io_t *file)
{
        if ((fwrite(file->buffer, 1, file->len, file->stream)) != file->len) {
                file_close(file);
                return SSUSB_FILE_IO_ERROR;
        }

        return SSUSB_OK;
}
