#include <assert.h>
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

static ssusb_ret_t
_file_exists(const char *input_file)
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
file_read(const char *input_file, void **buffer, size_t *len)
{
        assert(buffer != NULL);
        assert(len != NULL);

        ssusb_ret_t ret;
        ret = SSUSB_OK;

        *buffer = NULL;
        *len = 0;

        ret = _file_exists(input_file);
        if (ret != SSUSB_OK) {
                return ret;
        }

        FILE *file;

        if ((file = fopen(input_file, "rb+")) == NULL) {
                return _errno_convert();
        }

        /* Determine the size of file */
        if ((fseek(file, 0, SEEK_END)) < 0) {
                ret = _errno_convert();
                goto error;
        }
        long tell;
        if ((tell = ftell(file)) < 0) {
                ret = _errno_convert();
                goto error;
        }
        rewind(file);

        *len = tell;

        if (*len == 0) {
                ret = SSUSB_FILE_EMPTY;
                goto error;
        }

        if ((*buffer = malloc(*len)) == NULL) {
                ret = SSUSB_INSUFFICIENT_MEMORY;
                goto error;
        }
        (void)memset(*buffer, 0, *len);

        if ((fread(*buffer, 1, *len, file)) != *len) {
                ret = _errno_convert();
        }

        return ret;

error:
        fclose(file);

        return ret;
}

ssusb_ret_t
file_write(const char *output_file, const void *buffer, size_t len)
{
        assert(buffer != NULL);
        assert(len != 0);

        ssusb_ret_t ret;
        ret = _file_exists(output_file);
        if (ret != SSUSB_OK) {
                return ret;
        }

        FILE *file;

        if ((file = fopen(output_file, "wb+")) == NULL) {
                return _errno_convert();
        }

        if ((fwrite(buffer, 1, len, file)) != len) {
                ret = _errno_convert();
        }

        fclose(file);

        return ret;
}
