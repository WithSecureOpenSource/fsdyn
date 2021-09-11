/* Adapted from https://github.com/ulfjack/ryu commit
 * 6f85836b6389dce334692829d818cdedb28bfa00 ((C) Ulf Adams) under the
 * Apache 2.0 license. */

#ifdef RYU_ASSERT
#include <assert.h>
#define ASSERT(x) assert(x)
#else
#define ASSERT(x)
#endif
#include <errno.h>
#include <memory.h>
#include <string.h>

#include "float.h"
#include "float_tables.h"
#include "fsalloc.h"
#include "fsdyn_version.h"

/* float_parts_t is used both for base 2 and base 10 floats */
typedef struct {
    binary64_type_t type;
    bool negative;
    uint64_t significand;
    int32_t exponent;
} float_parts_t;

enum {
    DOUBLE_MANTISSA_BITS = 52,
    DOUBLE_EXPONENT_BITS = 11,
    DOUBLE_BIAS = 1023,
    DOUBLE_SIGN_BIT = DOUBLE_MANTISSA_BITS + DOUBLE_EXPONENT_BITS
};

static uint32_t log2pow5(uint32_t e)
{
    ASSERT(e <= 3528);
    return e * 1217359 >> 19;
}

static uint32_t ceil_log2pow5(uint32_t e)
{
    return log2pow5(e) + 1;
}

static uint32_t log10Pow2(uint32_t e)
{
    ASSERT(e <= 1650);
    return e * 78913 >> 18;
}

static uint32_t log10Pow5(uint32_t e)
{
    ASSERT(e <= 2620);
    return e * 732923 >> 20;
}

static uint64_t umul128(uint64_t a, uint64_t b, uint64_t *productHi)
{
    uint32_t aLo = a;
    uint32_t aHi = a >> 32;
    uint32_t bLo = b;
    uint32_t bHi = b >> 32;
    uint64_t b00 = (uint64_t) aLo * bLo;
    uint64_t b01 = (uint64_t) aLo * bHi;
    uint64_t b10 = (uint64_t) aHi * bLo;
    uint64_t b11 = (uint64_t) aHi * bHi;
    uint32_t b00Lo = b00;
    uint32_t b00Hi = b00 >> 32;
    uint64_t mid1 = b10 + b00Hi;
    uint32_t mid1Lo = mid1;
    uint32_t mid1Hi = mid1 >> 32;
    uint64_t mid2 = b01 + mid1Lo;
    uint32_t mid2Lo = mid2;
    uint32_t mid2Hi = mid2 >> 32;
    *productHi = b11 + mid1Hi + mid2Hi;
    return (uint64_t) mid2Lo << 32 | b00Lo;
}

static uint64_t shiftright128(uint64_t lo, uint64_t hi, uint32_t dist)
{
    ASSERT(dist < 64);
    return hi << 64 - dist | lo >> dist;
}

/* Return true if value is divisible by 5**p. */
static bool multipleOfPowerOf5(uint64_t value, uint32_t p)
{
    if (!p)
        return true;
    uint64_t m_inv_5 = 14757395258967641293U; /* inverse of 5 (mod 2**64)*/
    uint64_t n_div_5 = 3689348814741910323U;  /* 2**64 div 5 */
    uint32_t count = 0;
    for (;;) {
        ASSERT(value != 0);
        value *= m_inv_5;
        if (value > n_div_5)
            return false;
        if (++count >= p)
            return true;
    }
}

/* Return true if value is divisible by 2**p. */
static bool multipleOfPowerOf2(uint64_t value, uint32_t p)
{
    ASSERT(value != 0);
    ASSERT(p < 64);
    return (value & (((uint64_t) 1 << p) - 1)) == 0;
}

static uint64_t mulShift64(uint64_t m, const uint64_t *mul, int32_t j)
{
    /* m is maximum 55 bits */
    uint64_t high1;
    uint64_t low1 = umul128(m, mul[1], &high1);
    uint64_t high0;
    umul128(m, mul[0], &high0);
    uint64_t sum = high0 + low1;
    high1 += sum < high0;
    return shiftright128(sum, high1, j - 64);
}

static uint64_t mulShiftAll64(uint64_t m, const uint64_t *mul, int32_t j,
                              uint64_t *vp, uint64_t *vm, uint32_t mmShift)
{
    m <<= 1;
    /* m is maximum 55 bits */
    uint64_t tmp;
    uint64_t lo = umul128(m, mul[0], &tmp);
    uint64_t hi;
    uint64_t mid = tmp + umul128(m, mul[1], &hi);
    hi += mid < tmp;
    uint64_t lo2 = lo + mul[0];
    uint64_t mid2 = mid + mul[1] + (lo2 < lo);
    uint64_t hi2 = hi + (mid2 < mid);
    *vp = shiftright128(mid2, hi2, (uint32_t)(j - 64 - 1));
    if (mmShift == 1) {
        uint64_t lo3 = lo - mul[0];
        uint64_t mid3 = mid - mul[1] - (lo3 > lo);
        uint64_t hi3 = hi - (mid3 > mid);
        *vm = shiftright128(mid3, hi3, (uint32_t)(j - 64 - 1));
    } else {
        uint64_t lo3 = lo + lo;
        uint64_t mid3 = mid + mid + (lo3 < lo);
        uint64_t hi3 = hi + hi + (mid3 < mid);
        uint64_t lo4 = lo3 - mul[0];
        uint64_t mid4 = mid3 - mul[1] - (lo4 > lo3);
        uint64_t hi4 = hi3 - (mid4 > mid3);
        *vm = shiftright128(mid4, hi4, (uint32_t)(j - 64));
    }
    return shiftright128(mid, hi, (uint32_t)(j - 64 - 1));
}

static uint32_t floor_log2(uint64_t value)
{
    return 63 - __builtin_clzll(value);
}

static bool ryu_encode(const float_parts_t *dec, uint64_t *result)
{
    int32_t e2;
    uint64_t m2;
    bool trailingZeros;
    if (dec->exponent >= 0) {
        e2 = floor_log2(dec->significand) + dec->exponent +
            log2pow5(dec->exponent) - (DOUBLE_MANTISSA_BITS + 1);
        int j = e2 - dec->exponent - ceil_log2pow5(dec->exponent) +
            DOUBLE_POW5_BITCOUNT;
        ASSERT(j >= 0);
        ASSERT(dec->exponent < DOUBLE_POW5_TABLE_SIZE);
        m2 = mulShift64(dec->significand, DOUBLE_POW5_SPLIT[dec->exponent], j);
        trailingZeros = e2 < dec->exponent ||
            e2 - dec->exponent < 64 &&
                multipleOfPowerOf2(dec->significand, e2 - dec->exponent);
    } else {
        e2 = floor_log2(dec->significand) + dec->exponent -
            ceil_log2pow5(-dec->exponent) - (DOUBLE_MANTISSA_BITS + 1);
        int j = e2 - dec->exponent + ceil_log2pow5(-dec->exponent) - 1 +
            DOUBLE_POW5_INV_BITCOUNT;
        ASSERT(-dec->exponent < DOUBLE_POW5_INV_TABLE_SIZE);
        m2 = mulShift64(dec->significand, DOUBLE_POW5_INV_SPLIT[-dec->exponent],
                        j);
        trailingZeros = multipleOfPowerOf5(dec->significand, -dec->exponent);
    }
    uint32_t ieee_e2 = e2 + DOUBLE_BIAS + floor_log2(m2);
    if ((int32_t) ieee_e2 < 0)
        ieee_e2 = 0;
    if (ieee_e2 > 0x7fe)
        return false;
    int32_t shift = (ieee_e2 ?: 1) - e2 - DOUBLE_BIAS - DOUBLE_MANTISSA_BITS;
    ASSERT(shift >= 0);
    trailingZeros &= (m2 & (((uint64_t) 1 << (shift - 1)) - 1)) == 0;
    uint64_t lastRemovedBit = (m2 >> (shift - 1)) & 1;
    bool roundUp = lastRemovedBit && (!trailingZeros || m2 >> shift & 1);
    uint64_t ieee_m2 = (m2 >> shift) + roundUp;
    ASSERT(ieee_m2 <= ((uint64_t) 1 << (DOUBLE_MANTISSA_BITS + 1)));
    ieee_m2 &= ((uint64_t) 1 << DOUBLE_MANTISSA_BITS) - 1;
    if (ieee_m2 == 0 && roundUp)
        ieee_e2++;
    *result = (uint64_t) dec->negative << DOUBLE_SIGN_BIT |
        (uint64_t) ieee_e2 << DOUBLE_MANTISSA_BITS | ieee_m2;
    return true;
}

static const char *parse_nan(const char *p, const char *end, float_parts_t *dec)
{
    dec->type = BINARY64_TYPE_NAN;
    p++;
    if (p == end || *p != 'a' && *p != 'A')
        return NULL;
    p++;
    if (p == end || *p != 'n' && *p != 'N')
        return NULL;
    return p + 1;
}

static const char *parse_infinity(const char *p, const char *end,
                                  float_parts_t *dec)
{
    dec->type = BINARY64_TYPE_INFINITY;
    p++;
    if (p == end || *p != 'n' && *p != 'N')
        return NULL;
    p++;
    if (p == end || *p != 'f' && *p != 'F')
        return NULL;
    const char *ret = ++p;
    if (p == end || *p != 'i' && *p != 'I')
        return ret;
    p++;
    if (p == end || *p != 'n' && *p != 'N')
        return ret;
    p++;
    if (p == end || *p != 'i' && *p != 'I')
        return ret;
    p++;
    if (p == end || *p != 't' && *p != 'T')
        return ret;
    p++;
    if (p == end || *p != 'y' && *p != 'Y')
        return ret;
    return p + 1;
}

static const char *parse_big_unsigned(const char *p, const char *end,
                                      const char **start, uint64_t *value,
                                      int *magnitude, bool *exact)
{
    if (p == end || *p < '0' || *p > '9')
        return NULL;
    uint64_t n = *value;
    int m = *magnitude;
    if (!n) /* skip leading zeros */
        while (p != end && *p == '0')
            p++;
    if (!*start)
        *start = p;
    while (p != end && *p >= '0' && *p <= '9') {
        char digit = *p++ - '0';
        uint64_t mul10 = 10 * n;
        uint64_t next = mul10 + digit;
        if (n > (uint64_t) -1 / 10 || next < mul10) { /* overflow? */
            *exact = false;
            if (digit >= 5) {                         /* round up? */
                if (n < (uint64_t) -1) {
                    n++;
                } else {
                    n = n / 10 + 1;
                    m--;
                }
            }
            break;
        }
        n = next;
        m++;
    }
    for (; p != end && *p >= '0' && *p <= '9'; p++)
        ;
    *value = n;
    *magnitude = m;
    return p;
}

static const char *normalize(const char *p, float_parts_t *dec)
{
    uint32_t e = dec->exponent;
    while (dec->significand % 100 == 0) {
        dec->significand /= 100;
        e += 2;
    }
    if (dec->significand % 10 == 0) {
        dec->significand /= 10;
        e++;
    }
    if ((int32_t) e < dec->exponent) /* overflow? */
        return NULL;
    dec->exponent = e;
    return p;
}

static const char *parse_significand(const char *p, const char *end,
                                     float_parts_t *dec, bool *exact)
{
    dec->significand = 0;
    int magnitude = 0;
    const char *start = NULL;
    p = parse_big_unsigned(p, end, &start, &dec->significand, &magnitude,
                           exact);
    if (!p)
        return NULL;
    if (p == end || *p != '.') {
        if (!dec->significand)
            return p;
        size_t e = p - start - magnitude;
        dec->exponent = e;
        if (dec->exponent != e) /* overflow? */
            return NULL;
        return normalize(p, dec);
    }
    int integral_magnitude = magnitude;
    p = parse_big_unsigned(p + 1, end, &start, &dec->significand, &magnitude,
                           exact);
    if (!p)
        return NULL;
    if (!dec->significand)
        return p;
    if (integral_magnitude) {
        dec->exponent = integral_magnitude - magnitude;
        return normalize(p, dec);
    }
    size_t e = p - start - 1;
    dec->exponent = -e;
    if (-dec->exponent != e) /* overflow? */
        return NULL;
    return normalize(p, dec);
}

static const char *parse_normal(const char *p, const char *end,
                                float_parts_t *dec, bool *exact)
{
    p = parse_significand(p, end, dec, exact);
    if (!p)
        return NULL;
    if (p == end || *p != 'e' && *p != 'E') {
        dec->type =
            dec->significand ? BINARY64_TYPE_NORMAL : BINARY64_TYPE_ZERO;
        return p;
    }
    if (++p == end)
        return NULL;
    bool exp_sign = *p == '-';
    if (exp_sign || *p == '+')
        p++;
    const char *start = NULL;
    uint64_t exponent = 0;
    int magnitude = 0;
    bool exact_exp = true;
    p = parse_big_unsigned(p, end, &start, &exponent, &magnitude, &exact_exp);
    if (!p)
        return NULL;
    if (!dec->significand) {
        dec->type = BINARY64_TYPE_ZERO;
        return p;
    }
    if (exp_sign) {
        if (!exact_exp) { /* underflow? */
            errno = ERANGE;
            dec->type = BINARY64_TYPE_ZERO;
            return p;
        }
        int64_t e = dec->exponent - exponent;
        dec->exponent = e;
        if (dec->exponent != e) { /* underflow? */
            errno = ERANGE;
            dec->type = BINARY64_TYPE_ZERO;
            return p;
        }
        dec->type = BINARY64_TYPE_NORMAL;
        return p;
    }
    if (!exact_exp) { /* overflow? */
        errno = ERANGE;
        dec->type = BINARY64_TYPE_INFINITY;
        return p;
    }
    int64_t e = dec->exponent + exponent;
    dec->exponent = e;
    if (dec->exponent != e) { /* overflow? */
        errno = ERANGE;
        dec->type = BINARY64_TYPE_INFINITY;
        return p;
    }
    dec->type = BINARY64_TYPE_NORMAL;
    return p;
}

static const char *parse_float(const char *buffer, const char *end,
                               float_parts_t *dec, bool *exact)
{
    const char *p = buffer;
    if (p != end && (*p == '+' || *p == '-'))
        dec->negative = *p++ == '-';
    else
        dec->negative = 0;
    if (p == end)
        return NULL;
    switch (*p) {
        case 'N':
        case 'n':
            return parse_nan(p, end, dec);
        case 'I':
        case 'i':
            return parse_infinity(p, end, dec);
        default:
            return parse_normal(p, end, dec, exact);
    }
}

ssize_t binary64_parse_decimal(const char *buffer, size_t size,
                               binary64_type_t *type, bool *negative,
                               uint64_t *significand, int32_t *exponent,
                               bool *exact)
{
    float_parts_t dec;
    *exact = true;
    const char *p = parse_float(buffer, buffer + size, &dec, exact);
    if (!p) {
        errno = EILSEQ;
        return -1;
    }
    *type = dec.type;
    *negative = dec.negative;
    *significand = dec.significand;
    *exponent = dec.exponent;
    return p - buffer;
}

bool binary64_from_decimal(binary64_type_t type, bool negative,
                           uint64_t significand, int32_t exponent,
                           uint64_t *value)
{
    float_parts_t dec = { .type = type,
                          .negative = negative,
                          .significand = significand,
                          .exponent = exponent };
    return ryu_encode(&dec, value);
}

ssize_t binary64_from_string(const char *buffer, size_t size, uint64_t *value)
{
    float_parts_t dec;
    const char *end = buffer + size;
    bool exact = true;
    const char *p = parse_float(buffer, end, &dec, &exact);
    if (!p) {
        errno = EILSEQ;
        return -1;
    }
    switch (dec.type) {
        case BINARY64_TYPE_NAN:
            if (dec.negative)
                *value = BINARY64_CONST_S_NAN;
            else
                *value = BINARY64_CONST_Q_NAN;
            break;
        case BINARY64_TYPE_INFINITY:
            if (dec.negative)
                *value = BINARY64_CONST_NEG_INF;
            else
                *value = BINARY64_CONST_POS_INF;
            break;
        case BINARY64_TYPE_ZERO:
            if (dec.negative)
                *value = BINARY64_CONST_NEG_ZERO;
            else
                *value = BINARY64_CONST_ZERO;
            break;
        default:
            if (dec.exponent < -324) {
                if (dec.negative)
                    *value = BINARY64_CONST_NEG_ZERO;
                else
                    *value = BINARY64_CONST_ZERO;
                errno = ERANGE;
            } else if (dec.exponent > 308 || !ryu_encode(&dec, value)) {
                if (dec.negative)
                    *value = BINARY64_CONST_NEG_INF;
                else
                    *value = BINARY64_CONST_POS_INF;
                errno = ERANGE;
            } else if (*value == 0)
                errno = ERANGE;
    }
    return p - buffer;
}

static void decode_nontrivial(const float_parts_t *ieee, float_parts_t *dec)
{
    int32_t e2;
    uint64_t m2;
    if (ieee->exponent == 0) {
        e2 = 1 - DOUBLE_BIAS - DOUBLE_MANTISSA_BITS - 2;
        m2 = ieee->significand;
    } else {
        e2 = ieee->exponent - DOUBLE_BIAS - DOUBLE_MANTISSA_BITS - 2;
        m2 = (uint64_t) 1 << DOUBLE_MANTISSA_BITS | ieee->significand;
    }
    bool even = (m2 & 1) == 0;
    bool acceptBounds = even;
    uint64_t mv = 4 * m2;
    uint32_t mmShift = ieee->significand != 0 || ieee->exponent <= 1;
    uint64_t vr, vp, vm;
    int32_t e10;
    bool vmIsTrailingZeros = false;
    bool vrIsTrailingZeros = false;
    if (e2 >= 0) {
        uint32_t q = log10Pow2(e2) - (e2 > 3);
        e10 = q;
        int32_t k = DOUBLE_POW5_INV_BITCOUNT + log2pow5(q);
        int32_t i = -e2 + q + k;
        vr = mulShiftAll64(m2, DOUBLE_POW5_INV_SPLIT[q], i, &vp, &vm, mmShift);
        if (q <= 21) {
            if (mv % 5 == 0)
                vrIsTrailingZeros = multipleOfPowerOf5(mv, q);
            else if (acceptBounds)
                vmIsTrailingZeros = multipleOfPowerOf5(mv - 1 - mmShift, q);
            else
                vp -= multipleOfPowerOf5(mv + 2, q);
        }
    } else {
        uint32_t q = log10Pow5(-e2) - (-e2 > 1);
        e10 = q + e2;
        int32_t i = -e2 - q;
        int32_t k = ceil_log2pow5(i) - DOUBLE_POW5_BITCOUNT;
        int32_t j = q - k;
        vr = mulShiftAll64(m2, DOUBLE_POW5_SPLIT[i], j, &vp, &vm, mmShift);
        if (q <= 1) {
            vrIsTrailingZeros = true;
            if (acceptBounds)
                vmIsTrailingZeros = mmShift == 1;
            else
                --vp;
        } else if (q < 63) /* TODO(ulfjack): Use a tighter bound here. */
            vrIsTrailingZeros = multipleOfPowerOf2(mv, q);
    }
    int32_t removed = 0;
    uint8_t lastRemovedDigit = 0;
    if (vmIsTrailingZeros || vrIsTrailingZeros) {
        for (;;) {
            uint64_t vpDiv10 = vp / 10;
            uint64_t vmDiv10 = vm / 10;
            if (vpDiv10 <= vmDiv10)
                break;
            uint32_t vmMod10 = vm % 10;
            uint64_t vrDiv10 = vr / 10;
            uint32_t vrMod10 = vr % 10;
            vmIsTrailingZeros &= vmMod10 == 0;
            vrIsTrailingZeros &= lastRemovedDigit == 0;
            lastRemovedDigit = vrMod10;
            vr = vrDiv10;
            vp = vpDiv10;
            vm = vmDiv10;
            ++removed;
        }
        if (vmIsTrailingZeros) {
            for (;;) {
                uint64_t vmDiv10 = vm / 10;
                uint32_t vmMod10 = vm % 10;
                if (vmMod10 != 0)
                    break;
                uint64_t vpDiv10 = vp / 10;
                uint64_t vrDiv10 = vr / 10;
                uint32_t vrMod10 = vr % 10;
                vrIsTrailingZeros &= lastRemovedDigit == 0;
                lastRemovedDigit = vrMod10;
                vr = vrDiv10;
                vp = vpDiv10;
                vm = vmDiv10;
                ++removed;
            }
        }
        if (vrIsTrailingZeros && lastRemovedDigit == 5 && vr % 2 == 0)
            lastRemovedDigit = 4;
        dec->significand = vr +
            ((vr == vm && (!acceptBounds || !vmIsTrailingZeros)) ||
             lastRemovedDigit >= 5);
    } else {
        bool roundUp = false;
        uint64_t vpDiv100 = vp / 100;
        uint64_t vmDiv100 = vm / 100;
        if (vpDiv100 > vmDiv100) {
            uint64_t vrDiv100 = vr / 100;
            uint32_t vrMod100 = vr % 100;
            roundUp = vrMod100 >= 50;
            vr = vrDiv100;
            vp = vpDiv100;
            vm = vmDiv100;
            removed += 2;
        }
        for (;;) {
            uint64_t vpDiv10 = vp / 10;
            uint64_t vmDiv10 = vm / 10;
            if (vpDiv10 <= vmDiv10)
                break;
            uint64_t vrDiv10 = vr / 10;
            uint32_t vrMod10 = vr % 10;
            roundUp = vrMod10 >= 5;
            vr = vrDiv10;
            vp = vpDiv10;
            vm = vmDiv10;
            ++removed;
        }
        dec->significand = vr + (vr == vm || roundUp);
    }
    dec->exponent = e10 + removed;
}

static void ieee64_breakup(uint64_t value, float_parts_t *ieee)
{
    ieee->negative = value >> DOUBLE_MANTISSA_BITS + DOUBLE_EXPONENT_BITS & 1;
    ieee->significand = value & ((uint64_t) 1 << DOUBLE_MANTISSA_BITS) - 1;
    ieee->exponent = (value >> DOUBLE_MANTISSA_BITS &
                      ((uint32_t) 1 << DOUBLE_EXPONENT_BITS) - 1);
    if (ieee->exponent == (((uint32_t) 1 << DOUBLE_EXPONENT_BITS) - 1)) {
        if (ieee->significand)
            ieee->type = BINARY64_TYPE_NAN;
        else
            ieee->type = BINARY64_TYPE_INFINITY;
    } else if (ieee->exponent || ieee->significand)
        ieee->type = BINARY64_TYPE_NORMAL;
    else
        ieee->type = BINARY64_TYPE_ZERO;
}

static void ryu_decode(uint64_t value, float_parts_t *dec)
{
    float_parts_t ieee;
    ieee64_breakup(value, &ieee);
    dec->type = ieee.type;
    dec->negative = ieee.negative;
    if (dec->type != BINARY64_TYPE_NORMAL)
        return;
    int32_t e2 = DOUBLE_BIAS + DOUBLE_MANTISSA_BITS - ieee.exponent;
    if (e2 >= 0 && e2 <= 52) {
        uint64_t m2 = (uint64_t) 1 << DOUBLE_MANTISSA_BITS | ieee.significand;
        if (!(m2 & ((uint64_t) 1 << e2) - 1)) {
            dec->significand = m2 >> e2;
            dec->exponent = 0;
            (void) normalize(NULL, dec);
            return;
        }
    }
    decode_nontrivial(&ieee, dec);
}

void binary64_to_decimal(uint64_t value, binary64_type_t *type, bool *negative,
                         uint64_t *significand, int32_t *exponent)
{
    float_parts_t dec;
    ryu_decode(value, &dec);
    *type = dec.type;
    *negative = dec.negative;
    *significand = dec.significand;
    *exponent = dec.exponent;
}

static void emit_1_digit(char *p, uint32_t n)
{
    *p = '0' + n % 10;
}

/* A table of all two-digit numbers. */
static const char *DIGIT_TABLE = "0001020304050607080910111213141516171819"
                                 "2021222324252627282930313233343536373839"
                                 "4041424344454647484950515253545556575859"
                                 "6061626364656667686970717273747576777879"
                                 "8081828384858687888990919293949596979899";

static void emit_2_digits(char *p, uint32_t n)
{
    memcpy(p, &DIGIT_TABLE[n % 100 * 2], 2);
}

static void emit_4_digits(char *p, uint32_t n)
{
    emit_2_digits(p + 2, n);
    emit_2_digits(p, n / 100);
}

static void emit_8_digits(char *p, uint32_t n)
{
    emit_4_digits(p + 4, n);
    emit_4_digits(p, n / 10000);
}

static void emit_integer(char *p, uint64_t n, int magnitude)
{
    while (magnitude >= 8) {
        uint32_t n_low = n % 100000000;
        n /= 100000000;
        magnitude -= 8;
        emit_8_digits(p + magnitude, n_low);
    }
    uint32_t n_low = n;
    if (magnitude >= 4) {
        magnitude -= 4;
        emit_4_digits(p + magnitude, n_low);
        n_low /= 10000;
    }
    if (magnitude >= 2) {
        magnitude -= 2;
        emit_2_digits(p + magnitude, n_low);
        n_low /= 100;
    }
    if (magnitude)
        emit_1_digit(p, n_low);
}

unsigned binary64_decimal_digits(uint64_t integer)
{
    unsigned slot = floor_log2(integer);
    return DECIMAL_BOUNDARIES[slot].lower +
        (integer >= DECIMAL_BOUNDARIES[slot].cutoff);
}

static size_t format_normal(const float_parts_t *dec, char buffer[])
{
    char *p = buffer;
    if (dec->negative)
        *p++ = '-';
    int significand_magnitude = binary64_decimal_digits(dec->significand);
    int magnitude = significand_magnitude + dec->exponent;
    if ((unsigned) magnitude + 5 <= 15) {
        /* Non-scientific notation */
        if (magnitude >= 1) {
            if (dec->exponent >= 0) {
                emit_integer(p, dec->significand, significand_magnitude);
                p += significand_magnitude;
                memset(p, '0', dec->exponent);
                p += dec->exponent;
            } else {
                emit_integer(p + 1, dec->significand, significand_magnitude);
                memmove(p, p + 1, magnitude);
                p[magnitude] = '.';
                p += significand_magnitude + 1;
            }
        } else {
            int leadup = 2 - magnitude;
            emit_integer(p + leadup, dec->significand, significand_magnitude);
            memset(p, '0', leadup);
            p[1] = '.';
            p += leadup + significand_magnitude;
        }
    } else {
        /* Scientific notation */
        emit_integer(p + 1, dec->significand, significand_magnitude);
        p[0] = p[1];
        *++p = '.';
        p += significand_magnitude;
        *p++ = 'e';
        int exp = dec->exponent + significand_magnitude - 1;
        if (exp >= 0) {
            *p++ = '+';
        } else {
            *p++ = '-';
            exp = -exp;
        }
        if (exp < 100)
            emit_2_digits(p, exp);
        else if (exp < 1000) {
            emit_1_digit(p++, exp / 100);
            emit_2_digits(p, exp);
        } else {
            emit_2_digits(p, exp / 100);
            p += 2;
            emit_2_digits(p, exp);
        }
        p += 2;
    }
    *p = '\0';
    return p - buffer;
}

size_t binary64_format(uint64_t value, char buffer[BINARY64_MAX_FORMAT_SPACE])
{
    float_parts_t dec;
    ryu_decode(value, &dec);
    if (dec.type == BINARY64_TYPE_NORMAL)
        return format_normal(&dec, buffer);
    if (dec.negative)
        switch (dec.type) {
            case BINARY64_TYPE_NAN:
                strcpy(buffer, "-nan");
                return 4;
            case BINARY64_TYPE_INFINITY:
                strcpy(buffer, "-infinity");
                return 9;
            default:
                strcpy(buffer, "-0.0");
                return 4;
        }
    switch (dec.type) {
        case BINARY64_TYPE_NAN:
            strcpy(buffer, "nan");
            return 3;
        case BINARY64_TYPE_INFINITY:
            strcpy(buffer, "infinity");
            return 8;
        default:
            strcpy(buffer, "0");
            return 1;
    }
}

char *binary64_to_string(uint64_t value)
{
    char *s = fsalloc(BINARY64_MAX_FORMAT_SPACE);
    (void) binary64_format(value, s);
    return s;
}
