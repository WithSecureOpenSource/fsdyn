#ifndef __FSDYN_BYTEARRAY__
#define __FSDYN_BYTEARRAY__

#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct byte_array byte_array_t;

/*
 * A byte array maintains a dynamic sequence of bytes with a trailing
 * NUL byte. This allows applications to use this data structure also
 * for managing dynamic C strings. The array can store a maximum of
 * max_size bytes, including the NUL terminator. The bytes are stored
 * contiguously.
 */
byte_array_t *make_byte_array(size_t max_size);
byte_array_t *share_byte_array(byte_array_t *array);
void destroy_byte_array(byte_array_t *array);

typedef ssize_t (*byte_array_read_cb)(void *obj, void *buf, size_t count);

bool byte_array_copy(byte_array_t *array,
                     size_t pos,
                     const void *data,
                     size_t len);
bool byte_array_copy_string(byte_array_t *array, size_t pos, const char *str);
bool byte_array_append(byte_array_t *array, const void *data, size_t len);
bool byte_array_append_string(byte_array_t *array, const char *str);
ssize_t byte_array_append_stream(byte_array_t *array,
                                 byte_array_read_cb read_cb,
                                 void *obj,
                                 size_t len);
void byte_array_clear(byte_array_t *array);
/*
 * If n is greater than the current size, the array is expanded as
 * needed with bytes equal to c. If n is smaller, the array sequence
 * is set to its first n bytes.
 */
bool byte_array_resize(byte_array_t *array, size_t n, uint8_t c);

const void *byte_array_data(byte_array_t *array);
size_t byte_array_size(byte_array_t *array);

#ifdef __cplusplus
}
#endif

#endif
