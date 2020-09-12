#include <fsdyn/float.h>
#include <limits.h>
#include <math.h>
#include <stdlib.h>

static bool double_to_integer(double value, long long *n)
{
    union {
        double f;
        uint64_t i;
    } _value;
    _value.f = value;
    return binary64_to_integer(_value.i, n);
}

static bool double_to_unsigned(double value, unsigned long long *n)
{
    union {
        double f;
        uint64_t i;
    } _value;
    _value.f = value;
    return binary64_to_unsigned(_value.i, n);
}

static bool test_integer(void)
{
    long long n;
    if (double_to_integer(M_PI, &n))
        return false;
    if (double_to_integer(-M_PI, &n))
        return false;
    if (double_to_integer(NAN, &n))
        return false;
    if (double_to_integer(INFINITY, &n))
        return false;
    if (double_to_integer(0.1, &n))
        return false;
    if (double_to_integer(-0.1, &n))
        return false;
    if (double_to_integer(1e20, &n))
        return false;
    if (double_to_integer(-1e20, &n))
        return false;
    if (!double_to_integer(0.0, &n) || n != 0)
        return false;
    if (!double_to_integer(127.0, &n) || n != 127)
        return false;
    if (!double_to_integer(-127.0, &n) || n != -127)
        return false;
    if (!double_to_integer((1ULL << 53) + 1, &n))
        return false;
    return true;
}

static bool test_unsigned(void)
{
    unsigned long long n;
    if (double_to_unsigned(M_PI, &n))
        return false;
    if (double_to_unsigned(-M_PI, &n))
        return false;
    if (double_to_unsigned(NAN, &n))
        return false;
    if (double_to_unsigned(INFINITY, &n))
        return false;
    if (double_to_unsigned(0.1, &n))
        return false;
    if (double_to_unsigned(-0.1, &n))
        return false;
    if (double_to_unsigned(1e20, &n))
        return false;
    if (double_to_unsigned(-1e20, &n))
        return false;
    if (!double_to_unsigned(0.0, &n) || n != 0)
        return false;
    if (!double_to_unsigned(127.0, &n) || n != 127)
        return false;
    if (double_to_unsigned(-127.0, &n))
        return false;
    if (!double_to_unsigned((1ULL << 53) + 1, &n))
        return false;
    return true;
}

int main()
{
    if (!test_integer())
        return EXIT_FAILURE;
    if (!test_unsigned())
        return EXIT_FAILURE;
    return EXIT_SUCCESS;
}
