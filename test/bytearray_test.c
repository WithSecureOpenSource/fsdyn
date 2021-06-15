#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <fsdyn/bytearray.h>

static ssize_t read_bytes(void *obj, void *buf, size_t count)
{
    const char data[] = "foobarbaz";
    size_t len = strlen(data);
    if (count > len)
        count = len;
    memcpy(buf, data, count);
    return count;
}

static bool test_bytearray(void)
{
    byte_array_t *array = make_byte_array(8);

    if (!byte_array_append_string(array, "foo"))
        return false;
    if (byte_array_size(array) != 3)
        return false;
    if (memcmp(byte_array_data(array), "foo", 3))
        return false;

    if (!byte_array_append_string(array, "bar"))
        return false;
    if (byte_array_size(array) != 6)
        return false;
    if (memcmp(byte_array_data(array), "foobar", 6))
        return false;

    if (byte_array_append_string(array, "bar"))
        return false;

    byte_array_clear(array);
    if (byte_array_copy_string(array, 1, "foobar"))
        return false;

    if (!byte_array_copy_string(array, 0, "foo"))
        return false;

    if (!byte_array_copy_string(array, byte_array_size(array), "bar"))
        return false;

    if (!byte_array_copy(array, 1, byte_array_data(array), 3))
        return false;

    if (strcmp(byte_array_data(array), "ffooar"))
        return false;

    if (!byte_array_copy_string(array, 4, "bar"))
        return false;

    if (strcmp(byte_array_data(array), "ffoobar"))
        return false;

    byte_array_clear(array);
    if (!byte_array_append_string(array, "foobar"))
        return false;

    if (!byte_array_resize(array, 3, 0))
        return false;

    if (strcmp(byte_array_data(array), "foo"))
        return false;

    if (!byte_array_resize(array, 7, 'a'))
        return false;

    if (strcmp(byte_array_data(array), "fooaaaa"))
        return false;

    byte_array_clear(array);
    if (byte_array_size(array) != 0)
        return false;

    if (byte_array_append_stream(array, read_bytes, NULL, 8) != 7)
        return false;

    if (byte_array_append_stream(array, read_bytes, NULL, 8) >= 0 ||
        errno != ENOSPC)
        return false;

    destroy_byte_array(array);

    return true;
}

int main()
{
    if (!test_bytearray())
        return EXIT_FAILURE;
    return EXIT_SUCCESS;
}
