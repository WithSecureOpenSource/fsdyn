#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <fsdyn/float.h>

static const char *number[] = { "0",
                                "7",
                                "712",
                                "700",
                                "712.292",
                                "2333444712",
                                "3.33444712e-07",
                                "3.33444712e+10",
                                "0.1",
                                "0.123",
                                "0.00000123",
                                "-2",
                                "-2333444712",
                                "-3.33444712",
                                "-3.33444712e-07",
                                "-3.33444712e-10",
                                "-0.00000123",
                                "nan",
                                "-nan",
                                "infinity",
                                "-infinity",
                                "-0.0",
                                NULL };

static bool test_parse(const char *number, double value, const char *end)
{
    union {
        uint64_t bits;
        double value;
    } v;
    ssize_t count = binary64_from_string(number, -1, &v.bits);
    if (count < 0) {
        fprintf(stderr, "Failed to parse: \"%s\"\n", number);
        return false;
    }
    if (count != strlen(number)) {
        fprintf(stderr, "Premature end to parsing: \"%s\" (%d)\n", number,
                (int) count);
        return false;
    }
    binary64_float_t dec;
    (void) binary64_to_decimal(v.bits, &dec);
    switch (dec.type) {
        case BINARY64_TYPE_NAN:
        case BINARY64_TYPE_INFINITY:
        case BINARY64_TYPE_ZERO:
            break;
        default: {
            if (value != v.value) {
                fprintf(stderr, "Parse mismatch: \"%s\" --> %.21g | %.21g\n",
                        number, v.value, value);
                return false;
            }
        }
    }
    return true;
}

static bool test_format(const char *number, double value)
{
    char repr[BINARY64_MAX_FORMAT_SPACE];
    union {
        uint64_t bits;
        double value;
    } v = { .value = value };
    size_t length = binary64_format(v.bits, repr);
    if (strcmp(number, repr)) {
        fprintf(stderr, "Formatting failure: \"%s\" --> \"%s\"\n", number,
                repr);
        return false;
    }
    if (strlen(repr) != length) {
        fprintf(stderr, "Formatting failure: |\"%s\"| --> %d\n", number,
                (int) length);
        return false;
    }
    return true;
}

static bool test_num(const char *number)
{
    char *end;
    double value = strtod(number, &end);
    if (!test_parse(number, value, end))
        return false;
    if (!test_format(number, value))
        return false;
    return true;
}

static bool test_random_formatting()
{
    int fd = open("/dev/urandom", O_RDONLY);
    assert(fd >= 0);
    for (int i = 0; i < 100000; i++) {
        union {
            uint64_t bits;
            double value;
        } v;
        read(fd, &v.bits, sizeof v.bits);
        char repr[BINARY64_MAX_FORMAT_SPACE];
        size_t length = binary64_format(v.bits, repr);
        double value2 = strtod(repr, NULL);
        if (v.value != value2 &&
            !(isnan(v.value) && isnan(v.value) &&
              signbit(v.value) == signbit(v.value))) {
            fprintf(stderr,
                    "Failure: %.21g --> \"%s\" (instead of \"%.21g\")\n",
                    v.value, repr, value2);
            close(fd);
            return false;
        }
        if (strlen(repr) != length) {
            fprintf(stderr, "Formatting failure: |\"%s\"| --> %d\n", repr,
                    (int) length);
            return false;
        }
    }
    close(fd);
    return true;
}

static bool test_overflow(const char *number)
{
    union {
        uint64_t bits;
        double value;
    } v;
    errno = 0;
    ssize_t count = binary64_from_string(number, -1, &v.bits);
    if (count < 0) {
        fprintf(stderr, "Unexpected parsing failure: \"%s\"\n", number);
        return false;
    }
    if (errno != ERANGE) {
        fprintf(stderr, "Unexpected errno: \"%s\" %d (%s)\n", number, errno,
                strerror(errno));
        return false;
    }
    if (!isinf(v.value)) {
        fprintf(stderr, "Not infinite: \"%s\"\n", number);
        return false;
    }
    return true;
}

static bool test_underflow(const char *number)
{
    union {
        uint64_t bits;
        double value;
    } v;
    errno = 0;
    ssize_t count = binary64_from_string(number, -1, &v.bits);
    if (count < 0) {
        fprintf(stderr, "Unexpected parsing failure: \"%s\"\n", number);
        return false;
    }
    if (errno != ERANGE) {
        fprintf(stderr, "Unexpected errno: \"%s\" %d (%s)\n", number, errno,
                strerror(errno));
        return false;
    }
    if (v.value != 0.0) {
        fprintf(stderr, "Not zero: \"%s\"\n", number);
        return false;
    }
    return true;
}

static bool test_out_of_range()
{
    union {
        uint64_t bits;
        double value;
    } v;
    errno = 0;
    const char *number = "1e+100";
    ssize_t count = binary64_from_string(number, -1, &v.bits);
    if (count < 0) {
        fprintf(stderr, "Unexpected parsing failure: \"%s\"\n", number);
        return false;
    }
    if (errno != 0) {
        fprintf(stderr, "Unexpected overflow: \"%s\"\n", number);
        return false;
    }
    if (!test_overflow("1e10000"))
        return false;
    if (!test_overflow("1.797693134863e+308"))
        return false;
    if (!test_underflow("-1e-10000"))
        return false;
    if (!test_underflow("2e-324"))
        return false;
    return true;
}

int main()
{
    for (int i = 0; number[i]; i++)
        if (!test_num(number[i]))
            return EXIT_FAILURE;
    if (!test_random_formatting())
        return EXIT_FAILURE;
    if (!test_out_of_range())
        return EXIT_FAILURE;
    return EXIT_SUCCESS;
}
