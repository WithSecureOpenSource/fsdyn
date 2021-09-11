#ifndef __FSDYN_FLOAT__
#define __FSDYN_FLOAT__

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Useful IEEE 754 constants.
 */
#define BINARY64_CONST_Q_NAN     0x7ff8000000000000
#define BINARY64_CONST_S_NAN     0xfff8000000000000
#define BINARY64_CONST_POS_INF   0x7ff0000000000000
#define BINARY64_CONST_NEG_INF   0xfff0000000000000
#define BINARY64_CONST_ZERO      0x0000000000000000
#define BINARY64_CONST_NEG_ZERO  0x8000000000000000

/*
 * Try to cast a number in IEEE 754 binary64 floating point format to
 * a signed or unsigned integer. The functions fail if the number is
 * not an integer or if it cannot be represented in the target type.
 */
bool binary64_to_integer(uint64_t value, long long *n);
bool binary64_to_unsigned(uint64_t value, unsigned long long *n);

enum {
    BINARY64_MAX_FORMAT_SPACE = 25
};

/*
 * Convert a number in IEEE 754 binary64 floating point format to a
 * concise, NUL-terminated decimal representation that converts back
 * to the original value. The chosen format aims to be suitable for
 * human use in that exponential notation is used only for "very big"
 * or "very small" values.
 *
 * Special values are formatted to "-0.0", "nan", "-nan", "infinity"
 * and "-infinity".
 *
 * There are no bounds checks for buffer. The binary64_format_space()
 * function can be used to determine sufficient worst-case space
 * requirements.
 *
 * The returned value is strlen(3) of the result.
 */
size_t binary64_format(uint64_t value, char buffer[BINARY64_MAX_FORMAT_SPACE]);

/*
 * Return a dynamically allocated, NUL-terminated string in the format
 * given out by binary64_format() (q.v.). The return value should be
 * freed with fsfree().
 */
char *binary64_to_string(uint64_t value);

typedef enum {
    BINARY64_TYPE_NAN,
    BINARY64_TYPE_INFINITY,
    BINARY64_TYPE_ZERO,
    BINARY64_TYPE_NORMAL,
} binary64_type_t;

/*
 * Convert the binary64 floating point value to decimal, exponential
 * components. The components depend on the value type. The "negative"
 * argument is set for all value types. The other arguments are only
 * defined for BINARY64_TYPE_NORMAL.
 *
 * If case of BINARY64_TYPE_NORMAL, the components map back to value
 * as follows:
 *
 *    sprintf(buffer, "%s%llue%d",
 *            *negative ? "-" : "",
 *            (unsigned long long) *significand,
 *            (int) *exponent);
 *    value == strtod(buffer, NULL);
 *
 * Additionally, the significand is guaranteed not to be divisible by
 * 10.
 */
void binary64_to_decimal(uint64_t value,
                         binary64_type_t *type,
                         bool *negative,
                         uint64_t *significand,
                         int32_t *exponent);

/*
 * A convenience function to find the number of decimal digits needed
 * to represent the given unsigned integer. It can be used together
 * with binary64_to_decimal() to create alternative alternative
 * formatting outputs by determining the width of the significand and
 * the exponent.
 */
unsigned binary64_decimal_digits(uint64_t integer);

/*
 * Parse a valid decimal floating point representation starting at the
 * given string into a binary64 value. Parsing ends when the first
 * illegal character (e.g. NUL or punctuation) is encountered or the
 * end of the buffer is reached.
 *
 * A negative value is returned and errno is set to EILSEQ if the
 * representation is syntactically illegal. For example, 3 is returned
 * for "1.1}", but a negative number is returned for "1.}" because '.'
 * signals the beginning of a fraction.
 *
 * If the syntactically valid number is too big for the binary64
 * format, a positive value is returned, the value is set to an
 * infinity and errno is set to ERANGE. An application wishing to
 * guard for the possibility should set errno to zero before calling
 * the function. Analogously, an underflow results in a zero value
 * with errno set to ERANGE.
 */
ssize_t binary64_from_string(const char *buffer, size_t size, uint64_t *value);

/*
 * Create a binary64 floating point value corresponding to the given
 * arguments. If the value is too big for binary64, false is returned.
 * If the value is too small, a zero value is generated.
 */
bool binary64_from_decimal(binary64_type_t type,
                           bool negative,
                           uint64_t significand,
                           int32_t exponent,
                           uint64_t *value);

/*
 * A convenience function to parse a decimal floating point
 * representation starting at the given string into the given
 * arguments. See binary64_from_string() for the meaning of buffer,
 * size, the return value and errno settings. See
 * binary64_to_decimal() for the meaning of type, negative,
 * significand and exponent. Additionally, *exact is set to true if
 * *type == BINARY64_TYPE_NORMAL and *significand has not undergone
 * any rounding.
 */
ssize_t binary64_parse_decimal(const char *buffer, size_t size,
                               binary64_type_t *type, bool *negative,
                               uint64_t *significand, int32_t *exponent,
                               bool *exact);

#ifdef __cplusplus
}
#endif

#endif
