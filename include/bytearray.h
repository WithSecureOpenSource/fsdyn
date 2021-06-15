#ifndef __FSDYN_BYTEARRAY__
#define __FSDYN_BYTEARRAY__

#include <stdarg.h>
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

bool byte_array_copy(byte_array_t *array, size_t pos, const void *data,
                     size_t len);
bool byte_array_copy_string(byte_array_t *array, size_t pos, const char *str);
bool byte_array_append(byte_array_t *array, const void *data, size_t len);
bool byte_array_append_string(byte_array_t *array, const char *str);
bool byte_array_vappendf(byte_array_t *array, const char *fmt, va_list ap)
    __attribute__((format(printf, 2, 0)));
bool byte_array_appendf(byte_array_t *array, const char *fmt, ...)
    __attribute__((format(printf, 2, 3)));
ssize_t byte_array_append_stream(byte_array_t *array,
                                 byte_array_read_cb read_cb, void *obj,
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

#include <functional>
#include <memory>

namespace fsecure {
namespace fsdyn {

// std::unique_ptr for byte_array_t with custom deleter.
using ByteArrayPtr =
    std::unique_ptr<byte_array_t, std::function<void(byte_array_t *)>>;

// Create ByteArrayPtr that takes ownership of the provided byte_array_t. Pass
// nullptr to create an instance which doesn't contain any byte_array_t object.
inline ByteArrayPtr make_byte_array_ptr(byte_array_t *array)
{
    return { array, destroy_byte_array };
}

} // namespace fsdyn
} // namespace fsecure

#endif

#endif
