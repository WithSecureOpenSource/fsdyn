#ifndef __FSDYN_CHARSTR__
#define __FSDYN_CHARSTR__

#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <sys/types.h>
#include "fsalloc.h"
#include "list.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Like strcasecmp(3) except ASCII only (ie, no locale). */
int charstr_case_cmp(const char *a, const char *b);
int charstr_ncase_cmp(const char *a, const char *b, size_t n);

/* Return NULL if s does not start with prefix. Otherwise, return a
 * pointer to s after the prefix. */
const char *charstr_skip_prefix(const char *s, const char *prefix);

/* A case-insensitive version of charstr_skip(). */
const char *charstr_case_skip_prefix(const char *s, const char *prefix);

/* A badly named synonym of charstr_skip_prefix(). */
const char *charstr_case_starts_with(const char *s, const char *prefix);

/* A badly named synonym of charstr_case_skip_prefix(). */
const char *charstr_ncase_starts_with(const char *s, const char *prefix);

bool charstr_ends_with(const char *s, const char *suffix);

#define CHARSTR_CONTROL 0x0001   /* '\0'..'\037', '\177' */
#define CHARSTR_DIGIT   0x0002   /* '0'..'9' */
#define CHARSTR_LOWER   0x0004   /* 'a'..'z' */
#define CHARSTR_UPPER   0x0008   /* 'A'..'Z' */
#define CHARSTR_HT      0x0010   /* '\t' */
#define CHARSTR_LF      0x0020   /* '\n' */
#define CHARSTR_FF      0x0040   /* '\f' */
#define CHARSTR_CR      0x0080   /* '\r' */
#define CHARSTR_SPACE   0x0100   /* ' ' */
#define CHARSTR_OCTAL   0x0200   /* '0'..'7' */
#define CHARSTR_HEX     0x0400   /* '0'..'9', 'a'..'f', 'A'..'F' */
#define CHARSTR_UNDER   0x0800   /* '_' */
#define CHARSTR_PRINT   0x1000   /* ' '..'~' */

#define CHARSTR_TAB CHARSTR_HT
#define CHARSTR_NL CHARSTR_LF

#define CHARSTR_WHITESPACE \
  (CHARSTR_HT | CHARSTR_LF | CHARSTR_FF | CHARSTR_CR | CHARSTR_SPACE)
#define CHARSTR_ALPHA (CHARSTR_LOWER | CHARSTR_UPPER)
#define CHARSTR_ALNUM (CHARSTR_ALPHA | CHARSTR_DIGIT)
#define CHARSTR_ASCII (CHARSTR_CONTROL | CHARSTR_PRINT)

uint64_t charstr_char_class(char c);
char charstr_lcase_char(char c);
char charstr_ucase_char(char c);

/* The return value is well-defined only if c is a hexadecimal digit. */
unsigned charstr_digit_value(char c);

/* The returned string is allocated with fsalloc(). */
char *charstr_dupstr(const char *s);
char *charstr_dupsubstr(const char *s, const char *end);

/* Modify the argument string in place. Return s. */
char *charstr_lcase_str(char *s);
char *charstr_lcase_substr(char *s, const char *end);
char *charstr_ucase_str(char *s);
char *charstr_ucase_substr(char *s, const char *end);

/* Return a fresh list of strings. Each string is allocated with
 * fsalloc(). Specifying max_split = 0 returns a single-element list
 * containing a copy of s. Specify max_split = -1 to effectively remove
 * the split count limit. */
list_t *charstr_split(const char *s, char delim, unsigned max_split);

/* Split a string into an array. The size of the array must be at least
 * max_split + 1. Each string is allocated with fsalloc(). The value
 * returned is the number of splits. For example, if the function
 * returns 7, the last substring is found in array[7]. */
unsigned charstr_split_into_array(const char *s, char delim, char **array,
                                  unsigned max_split);

/* Return a fresh list of strings split at any nonempty sequence of
 * CHARSTR_WHITESPACE characters. If the argument only contains
 * whitespace, an empty list is returned. */
list_t *charstr_split_atoms(const char *s);

/* Like charstr_split(), but the delimiter is a string. */
list_t *charstr_split_str(const char *s, const char *delim,
                          unsigned max_split);

/* Return a copy of s with initial and final CHARSTR_WHITESPACE
 * characters stripped. The return value should be freed with
 * fsfree(). If s == NULL, NULL is returned. */
char *charstr_strip(const char *s);

/* Decode a single UTF-8-encode Unicode codepoint. The encoding begins
 * at s and is limited by end (which is the end of the buffer, not the
 * end of the codepoint encoding). If s is NULL, return NULL. If end is
 * NULL, there is no bounds check.
 *
 * If s points to an illegal UTF-8 encoding, return NULL. Surrogate
 * codepoints are considered illegal.
 *
 * If s points to a valid UTF-8 encoding, return a pointer to the byte
 * after the encoding, and assign the Unicode codepoint to *codepoint
 * (unless codepoint is NULL). */
const char *charstr_decode_utf8_codepoint(const char *s, const char *end,
                                          int *codepoint);

/* Return true iff the bounded string s points to a valid UTF-8
 * encoding. */
bool charstr_valid_utf8_bounded(const char *s, const char *end);

/* Return true iff the NUL-terminated string s points to a valid UTF-8
 * encoding. */
bool charstr_valid_utf8(const char *s);

/* Using fsalloc(), make a copy of the argument. Any byte that is not
 * valid UTF-8 is replaced with the (UTF-8 encoding of the) Unicode
 * replacement character (U+FFFD). */
char *charstr_sanitize_utf8(const char *s);

/* Encode a single Unicode codepoint using UTF-8. The end of the
 * output buffer is given (NULL means unenforced). The return value is
 * the point in the output buffer after the encoding. NULL is returned
 * if s == NULL or if the whole codepoint doesn't fit in the allotted
 * area. */
char *charstr_encode_utf8_codepoint(int codepoint, char *s, const char *end);

typedef enum {
    UNICODE_CATEGORY_Cn,        /* Other, not assigned */
    UNICODE_CATEGORY_Cc,        /* Other, control */
    UNICODE_CATEGORY_Cf,        /* Other, format */
    UNICODE_CATEGORY_Co,        /* Other, private use */
    UNICODE_CATEGORY_Cs,        /* Other, surrogate */
    UNICODE_CATEGORY_Ll,        /* Letter, lowercase */
    UNICODE_CATEGORY_Lm,        /* Letter, modifier */
    UNICODE_CATEGORY_Lo,        /* Letter, other */
    UNICODE_CATEGORY_Lt,        /* Letter, titlecase */
    UNICODE_CATEGORY_Lu,        /* Letter, uppercase */
    UNICODE_CATEGORY_Mc,        /* Mark, spacing combining */
    UNICODE_CATEGORY_Me,        /* Mark, enclosing */
    UNICODE_CATEGORY_Mn,        /* Mark, nonspacing */
    UNICODE_CATEGORY_Nd,        /* Number, decimal digit */
    UNICODE_CATEGORY_Nl,        /* Number, letter */
    UNICODE_CATEGORY_No,        /* Number, other */
    UNICODE_CATEGORY_Pc,        /* Punctuation, connector */
    UNICODE_CATEGORY_Pd,        /* Punctuation, dash */
    UNICODE_CATEGORY_Pe,        /* Punctuation, close */
    UNICODE_CATEGORY_Pf,        /* Punctuation, final quote */
    UNICODE_CATEGORY_Pi,        /* Punctuation, initial quote */
    UNICODE_CATEGORY_Po,        /* Punctuation, other */
    UNICODE_CATEGORY_Ps,        /* Punctuation, open */
    UNICODE_CATEGORY_Sc,        /* Symbol, currency */
    UNICODE_CATEGORY_Sk,        /* Symbol, modifier */
    UNICODE_CATEGORY_Sm,        /* Symbol, math */
    UNICODE_CATEGORY_So,        /* Symbol, other */
    UNICODE_CATEGORY_Zl,        /* Separator, line */
    UNICODE_CATEGORY_Zp,        /* Separator, paragraph */
    UNICODE_CATEGORY_Zs         /* Separator, space */
} charstr_unicode_category_t;

charstr_unicode_category_t charstr_unicode_category(int codepoint);

/* There is no language-independent way to convert between upper and
 * lower case. The following functions provide a simplistic
 * approximation. */
int charstr_naive_lcase_unicode(int codepoint);
int charstr_naive_ucase_unicode(int codepoint);

enum {
    /* UNICODE_*_DISALLOWED and UNICODE_*_MAYBE are mutually exclusive */
    UNICODE_NFC_DISALLOWED  = 0x01,
    UNICODE_NFC_MAYBE       = 0x02,
    UNICODE_NFD_DISALLOWED  = 0x04,
    UNICODE_NFD_MAYBE       = 0x08,
    UNICODE_NFKC_DISALLOWED = 0x10,
    UNICODE_NFKC_MAYBE      = 0x20,
    UNICODE_NFKD_DISALLOWED = 0x40,
    UNICODE_NFKD_MAYBE      = 0x80
};

/* You probably don't need this function. It is used internally in the
 * unicode normalization algorithms. The returned value is a bit
 * combination of the flags defined above. */
int charstr_allowed_unicode_normal_forms(int codepoint);

/* You probably don't need this function. It is used internally in the
 * unicode normalization algorithms. The returned value is an integer
 * ranging between 0 and 240. */
int charstr_unicode_canonical_combining_class(int codepoint);

/* You probably don't need this function. It is used internally in the
 * unicode normalization algorithms. A valid UTF-8 string is expected.
 * The string is terminated by a NUL byte or end, whichever limit
 * comes first. The two flags, disallowed_flag and maybe_flag, are a
 * pair of related constants from the enum above (for example,
 * UNICODE_NFC_DISALLOWED and UNICODE_NFC_MAYBE). Possible return
 * values: 0 ("allowed"), disallowed_flag ("disallowed"), maybe_flag
 * ("maybe"), -1 ("UTF-8 decoding error"). */
int charstr_detect_unicode_normal_form(const char *s, const char *end,
                                       int disallowed_flag, int maybe_flag);

/* If this function returns true, given string is valid UTF-8 and in
 * the NFC form. The converse is not necessarily true. */
bool charstr_unicode_canonically_composed(const char *s, const char *end);

/* If this function returns true, given string is valid UTF-8 and in
 * the NFD form. The converse is not necessarily true. */
bool charstr_unicode_canonically_decomposed(const char *s, const char *end);

/* Unconditionally copy the given UTF-8 string into a buffer
 * simultaneously performing a canonical NFD-normalization. The input
 * string is terminated by a NUL byte or end, whichever limit is
 * encountered first. The output buffer should be at least three times
 * the size of the input string (plus 1 for NUL termination) to
 * guarantee sufficient space.
 *
 * A non-NULL return value indicates the end of the generated UTF-8
 * output. The output is NUL-terminated. In other words, a non-NULL
 * return value points to the NUL-termination.
 *
 * A NULL value is returned and errno is set in case of an error:
 * EILSEQ indicates a badly encoded input string; EOVERFLOW indicates
 * insufficient output buffer space. */
char *charstr_unicode_decompose(const char *s, const char *end,
                                char output[], const char *output_end);

/* Unconditionally copy the given UTF-8 string into a buffer
 * simultaneously performing a canonical NFC-normalization. The input
 * string must already be in a canonical form (use
 * charstr_unicode_decompose() first if it is not). The string is
 * terminated by a NUL byte or end, whichever limit is encountered
 * first. The output buffer should be at least the same size as the
 * input buffer (plus 1 for NUL termination) to guarantee sufficient
 * space.
 *
 * A non-NULL return value indicates the end of the generated UTF-8
 * output. The output is NUL-terminated. In other words, a non-NULL
 * return value points to the NUL-termination.
 *
 * A NULL value is returned and errno is set in case of an error:
 * EILSEQ indicates a badly encoded input string; EOVERFLOW indicates
 * insufficient output buffer space. */
char *charstr_unicode_recompose(const char *s, const char *end,
                                char output[], const char *output_end);

/* Return a URL encoded string corresponding to the given byte:
 *  * ASCII 32 (SPC) yields "+"
 *  * digits and letters yield themselves as a one-character string
 *  * these punctuation characters yield themselves as a one-character string:
 *    '-', '.', '/', '<', '>', '\\', '^', '_', '`', '{', '|', ']', '~'
 *  * all other values yield the byte's percent encoding: "%XX" where X
 *    is an upper-case hexadecimal digit.
 */
const char *charstr_url_encode_byte(uint8_t byte);

/* Create a URL-encoding for the given NUL-terminated string.
 * The returned string is allocated using fsalloc(). */
char *charstr_url_encode(const char *string);

typedef struct charstr_url_encoder charstr_url_encoder_t;

/* Create a custom URL encoder. A custom URL encoder percent-encodes
 * every 8-bit character mentioned in the NUL-terminated 'reserve'
 * string, and it leaves unmodified every 8-bit character mentioned in
 * the NUL-terminated 'unreserve' string. Other characters are encoded
 * as documented for charstr_url_encode_byte().
 *
 * Note: the encoding of the NUL byte cannot be altered. */
charstr_url_encoder_t *charstr_create_url_encoder(const char *reserve,
                                                  const char *unreserve);

/* Deallocate the custom URL encoder returned by
 * charstr_create_url_encoder(). */
void charstr_destroy_url_encoder(charstr_url_encoder_t *encoder);

/* Return a URL encoded string corresponding to the given byte. */
const char *charstr_url_custom_encode_byte(charstr_url_encoder_t *encoder,
                                           uint8_t byte);

/* Create a URL-encoding for the given NUL-terminated string.
 * The returned string is allocated using fsalloc(). */
char *charstr_url_custom_encode(charstr_url_encoder_t *encoder,
                                const char *string);


/* Create a URL-decoding for the given NUL-terminated string.
 * The returned string is allocated using fsalloc().
 *
 * The '+' character is optionally decoded into ' '. If size is not
 * NULL, it will receive the decoded length, which is useful if the
 * decoding can contain "%00".
 *
 * If the argument string is not a legal URL-encoding, NULL is returned
 * and errno is set to EILSEQ. */
char *charstr_url_decode(const char *encoding, bool plus_is_space,
                         size_t *size);

/* Create a dynamically allocated string using the printf-style format
 * string. The result is allocated using fsalloc().
 *
 * If the argument string is not a legal format specifiation, NULL is
 * returned and errno is set to EILSEQ. */
char *charstr_printf(const char *format, ...)
    __attribute__((format(printf, 1, 2)));

/* The stdarg variant of charstr_printf(). */
char *charstr_vprintf(const char *format, va_list ap)
    __attribute__((format(printf, 1, 0)));

/* Encode the hostname using punycode. The argument must be valid
 * UTF-8 and also a valid Unicode hostname. Return an ASCII string or
 * NULL in case of an input error. The return value should be freed
 * using fsfree().
 *
 * Error checking is not bulletproof. In particular, if hostname is
 * all-ASCII, no validation is performed. A corollary: you can pass an
 * IPv4 address or a bracketed or unbracketed IPv6 address as a
 * hostname and get a copy back. */
char *charstr_idna_encode(const char *hostname);

/* You probably don't need these functions. They are used internally
 * in the punycode encoding. */
bool charstr_idna_status_is_deviation(int codepoint);
bool charstr_idna_status_is_disallowed(int codepoint);
bool charstr_idna_status_is_disallowed_STD3_valid(int codepoint);
bool charstr_idna_status_is_disallowed_STD3_mapped(int codepoint);
bool charstr_idna_status_is_ignored(int codepoint);
bool charstr_idna_status_is_mapped(int codepoint);
bool charstr_idna_status_is_valid(int codepoint);
const char *charstr_idna_status(int codepoint);

/* You probably don't need this function. It is used internally in
 * the punycode encoding. The return value is an UTF-8 string or
 * NULL. */
const char *charstr_idna_mapping(int codepoint);

#ifdef __cplusplus
}

#include <functional>
#include <memory>

namespace fsecure {
namespace fsdyn {

using CharstrUrlEncoderPtr =
    std::unique_ptr<charstr_url_encoder_t,
                    std::function<void(charstr_url_encoder_t *)>>;

inline CharstrUrlEncoderPtr make_charstr_url_encoder_ptr(
    charstr_url_encoder_t *encoder)
{
    return { encoder, charstr_destroy_url_encoder };
}

} // namespace fsdyn
} // namespace fsecure

#endif

#endif
