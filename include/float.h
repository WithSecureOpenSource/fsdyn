#ifndef __FSDYN_FLOAT__
#define __FSDYN_FLOAT__

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Try to cast a number in IEEE 754 binary64 floating point format to
 * a signed or unsigned integer. The functions fail if the number is
 * not an integer or if it cannot be represented in the target type.
 */
bool binary64_to_integer(uint64_t value, long long *n);
bool binary64_to_unsigned(uint64_t value, unsigned long long *n);

#ifdef __cplusplus
}
#endif

#endif
