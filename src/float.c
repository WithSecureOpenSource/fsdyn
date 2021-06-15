#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

bool binary64_to_integer(uint64_t value, long long *n)
{
    if (!(value << 1)) {
        *n = 0;
        return true;
    }

    if (value == 0xc3e0000000000000) {
        *n = LLONG_MIN;
        return true;
    }

    size_t bits = 8 * sizeof *n;
    int exp = (int) (value >> 52 & 0x7ff) - 1023;
    if (exp < 0 || exp >= bits - 1)
        return false;

    bool negative = value >> 63;
    uint64_t mask = ~0ULL >> 12;
    uint64_t significand = (value & mask) | 1ULL << 52;

    if (exp >= 52) {
        *n = significand << (exp - 52);
        if (negative)
            *n = -*n;
        return true;
    }

    if ((value & (mask >> exp)) == 0) {
        *n = significand >> (52 - exp);
        if (negative)
            *n = -*n;
        return true;
    }

    return false;
}

bool binary64_to_unsigned(uint64_t value, unsigned long long *n)
{
    if (!(value << 1)) {
        *n = 0;
        return true;
    }

    bool negative = value >> 63;
    if (negative)
        return false;

    size_t bits = 8 * sizeof *n;
    int exp = (int) (value >> 52 & 0x7ff) - 1023;
    if (exp < 0 || exp >= bits)
        return false;

    uint64_t mask = ~0ULL >> 12;
    uint64_t significand = (value & mask) | 1ULL << 52;

    if (exp >= 52) {
        *n = significand << (exp - 52);
        return true;
    }

    if ((value & (mask >> exp)) == 0) {
        *n = significand >> (52 - exp);
        return true;
    }

    return false;
}
