#include "bytearray.h"
#include "fsalloc.h"
#include "avltree_version.h"

#include <errno.h>
#include <string.h>

struct byte_array {
    uint8_t *data;
    size_t cursor;
    size_t max_size;
    size_t size;
};

byte_array_t *make_byte_array(size_t max_size)
{
    byte_array_t *array = fsalloc(sizeof *array);
    array->cursor = 0;
    array->max_size = max_size;
    array->size = 128;
    if (array->size > max_size)
        array->size = max_size;
    array->data = fsalloc(array->size);
    array->data[0] = 0;
    return array;
}

void destroy_byte_array(byte_array_t *array)
{
    fsfree(array->data);
    fsfree(array);
}

static bool ensure_space(byte_array_t *array, size_t len)
{
    if (len >= array->max_size - array->cursor) {
        errno = ENOSPC;
        return false;
    }

    if (len >= array->size - array->cursor) {
        size_t n = 1;

        /* round 'array->cursor + len + 1' to the minimum between
         * SIZE_MAX and its nearest power of two */
        while (n < array->cursor + len + 1) {
            if (n > SIZE_MAX / 2) {
                n = SIZE_MAX;
                break;
            }

            n <<= 1;
        }

        array->size = n;
        array->data = fsrealloc(array->data, array->size);
    }
    return true;
}

bool byte_array_copy(byte_array_t *array,
                     size_t pos,
                     const void *data,
                     size_t len)
{
    if (pos > array->cursor)
        return false;

    if (len > array->cursor - pos) {
        if (!ensure_space(array, pos + len - array->cursor))
            return false;
    }

    memmove(array->data + pos, data, len);
    if (len > array->cursor - pos) {
        array->cursor = pos + len;
        array->data[array->cursor] = 0;
    }
    return true;
}

bool byte_array_copy_string(byte_array_t *array, size_t pos, const char *str)
{
    return byte_array_copy(array, pos, str, strlen(str));
}

bool byte_array_append(byte_array_t *array, const void *data, size_t len)
{
    if (!ensure_space(array, len))
        return false;

    memcpy(array->data + array->cursor, data, len);
    array->cursor += len;
    array->data[array->cursor] = 0;
    return true;
}

bool byte_array_append_string(byte_array_t *array, const char *str)
{
    return byte_array_append(array, str, strlen(str));
}

ssize_t byte_array_append_stream(byte_array_t *array,
                                 byte_array_read_cb read_cb,
                                 void *obj,
                                 size_t len)
{
    size_t available = array->max_size - array->cursor;
    if (len >= available) {
        if (available == 1) {
            errno = ENOSPC;
            return -1;
        }
        len = available - 1;
    }
    if (!ensure_space(array, len))
        return -1;

    ssize_t count = read_cb(obj, array->data + array->cursor, len);
    if (count > 0) {
        array->cursor += count;
        array->data[array->cursor] = 0;
    }
    return count;
}

void byte_array_clear(byte_array_t *array)
{
    array->cursor = 0;
    array->data[0] = 0;
}

bool byte_array_resize(byte_array_t *array, size_t n, uint8_t c)
{
    if (n > array->cursor) {
        if (!ensure_space(array, n - array->cursor))
            return false;
        memset(array->data + array->cursor, c, n - array->cursor);
    }
    array->cursor = n;
    array->data[array->cursor] = 0;
    return true;
}

const void *byte_array_data(byte_array_t *array)
{
    return array->data;
}

size_t byte_array_size(byte_array_t *array)
{
    return array->cursor;
}
