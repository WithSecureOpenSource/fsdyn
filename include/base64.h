#ifndef __FSDYN_BASE64__
#define __FSDYN_BASE64__

#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>
#include "fsalloc.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Base64-encode binary input. The destination is NUL-terminated as
 * long as dest_size > 0. Returns the number of characters that would
 * be written, given a large enough buffer.
 *
 * If pos62 and/or pos63 is -1, the defaults '+' and '/' are used.
 *
 * This function cannot fail. */
size_t base64_encode_buffer(const void *source, size_t source_size,
                            char *dest, size_t dest_size,
                            char pos62, char pos63);

/* Return the size of the Base64 encoding (excluding the NUL
 * character) for the given number of bytes. Return -1 in case of an
 * overflow. */
size_t base64_encoding_size(size_t binary_size);

/* Create a freshly allocated Base64-encoded string. The return value
 * is NULL in case of an overflow. Otherwise, use fsfree() to free
 * it. */
char *base64_encode_simple(const void *buffer, size_t size);

/* Decode base64 input. The destination is not NUL-terminated. Returns
 * the number of bytes that would be written, given a large enough
 * buffer. The source is terminated if either source_size or NUL is
 * encountered. Specify -1 as source_size to only consider
 * NUL-termination.
 *
 * If pos62 and/or pos63 is -1, the defaults '+' and '/' are used.
 *
 * A negative return value indicates an error. Errno is set as follows:
 *
 *    EILSEQ: source is not correctly encoded
 *
 *    EOVERFLOW: the size of the decoding is larger than ssize_t can express
 */
ssize_t base64_decode_buffer(const char *source, size_t source_size,
                             void *dest, size_t dest_size,
                             char pos62, char pos63, bool ignore_wsp);

/* Create a freshly allocated binary blob from a NUL-terminated,
 * Base64-encoded string (with whitespace ignored). The return value
 * is NULL, and errno is set, in case of an error. Otherwise, use
 * fsfree() to free it. A complimentary NUL terminator is appended to
 * the decoding, but it is not included in the returned
 * binary_size. */
void *base64_decode_simple(const char *encoding, size_t *binary_size);

/* The standard Base64 encoding for 6-bit bitfields. 62 maps to '+'
 * and 63 maps to '/'. */
extern const char base64_bitfield_encoding[64];

/* The standard Base64 decoding for (unsigned) Base64 characters. '+'
 * maps to 62 and '/' maps to 63. Non-base64 characters map to
 * negative values. */
extern const int8_t base64_bitfield_decoding[256];

/* Special values in base64_bitfield_decoding[]. */
enum {
    BASE64_ILLEGAL_OTHER = -1,
    BASE64_ILLEGAL_WHITESPACE = -2,     /* ' \t\n\f\r' */
    BASE64_ILLEGAL_TRAILER = -3,        /* '=' */
    BASE64_ILLEGAL_NUL = -4
};

#ifdef __cplusplus
}
#endif

#endif
