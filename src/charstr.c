#include "charstr.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "bytearray.h"
#include "fsalloc.h"
#include "fsdyn_version.h"

static const struct {
    char lower, upper;
} case_table[256] = {
    { 0x00, 0x00 }, { 0x01, 0x01 }, { 0x02, 0x02 }, { 0x03, 0x03 },
    { 0x04, 0x04 }, { 0x05, 0x05 }, { 0x06, 0x06 }, { 0x07, 0x07 },
    { 0x08, 0x08 }, { 0x09, 0x09 }, { 0x0a, 0x0a }, { 0x0b, 0x0b },
    { 0x0c, 0x0c }, { 0x0d, 0x0d }, { 0x0e, 0x0e }, { 0x0f, 0x0f },
    { 0x10, 0x10 }, { 0x11, 0x11 }, { 0x12, 0x12 }, { 0x13, 0x13 },
    { 0x14, 0x14 }, { 0x15, 0x15 }, { 0x16, 0x16 }, { 0x17, 0x17 },
    { 0x18, 0x18 }, { 0x19, 0x19 }, { 0x1a, 0x1a }, { 0x1b, 0x1b },
    { 0x1c, 0x1c }, { 0x1d, 0x1d }, { 0x1e, 0x1e }, { 0x1f, 0x1f },
    { 0x20, 0x20 }, { 0x21, 0x21 }, { 0x22, 0x22 }, { 0x23, 0x23 },
    { 0x24, 0x24 }, { 0x25, 0x25 }, { 0x26, 0x26 }, { 0x27, 0x27 },
    { 0x28, 0x28 }, { 0x29, 0x29 }, { 0x2a, 0x2a }, { 0x2b, 0x2b },
    { 0x2c, 0x2c }, { 0x2d, 0x2d }, { 0x2e, 0x2e }, { 0x2f, 0x2f },
    { 0x30, 0x30 }, { 0x31, 0x31 }, { 0x32, 0x32 }, { 0x33, 0x33 },
    { 0x34, 0x34 }, { 0x35, 0x35 }, { 0x36, 0x36 }, { 0x37, 0x37 },
    { 0x38, 0x38 }, { 0x39, 0x39 }, { 0x3a, 0x3a }, { 0x3b, 0x3b },
    { 0x3c, 0x3c }, { 0x3d, 0x3d }, { 0x3e, 0x3e }, { 0x3f, 0x3f },
    { 0x40, 0x40 }, { 0x61, 0x41 }, { 0x62, 0x42 }, { 0x63, 0x43 },
    { 0x64, 0x44 }, { 0x65, 0x45 }, { 0x66, 0x46 }, { 0x67, 0x47 },
    { 0x68, 0x48 }, { 0x69, 0x49 }, { 0x6a, 0x4a }, { 0x6b, 0x4b },
    { 0x6c, 0x4c }, { 0x6d, 0x4d }, { 0x6e, 0x4e }, { 0x6f, 0x4f },
    { 0x70, 0x50 }, { 0x71, 0x51 }, { 0x72, 0x52 }, { 0x73, 0x53 },
    { 0x74, 0x54 }, { 0x75, 0x55 }, { 0x76, 0x56 }, { 0x77, 0x57 },
    { 0x78, 0x58 }, { 0x79, 0x59 }, { 0x7a, 0x5a }, { 0x5b, 0x5b },
    { 0x5c, 0x5c }, { 0x5d, 0x5d }, { 0x5e, 0x5e }, { 0x5f, 0x5f },
    { 0x60, 0x60 }, { 0x61, 0x41 }, { 0x62, 0x42 }, { 0x63, 0x43 },
    { 0x64, 0x44 }, { 0x65, 0x45 }, { 0x66, 0x46 }, { 0x67, 0x47 },
    { 0x68, 0x48 }, { 0x69, 0x49 }, { 0x6a, 0x4a }, { 0x6b, 0x4b },
    { 0x6c, 0x4c }, { 0x6d, 0x4d }, { 0x6e, 0x4e }, { 0x6f, 0x4f },
    { 0x70, 0x50 }, { 0x71, 0x51 }, { 0x72, 0x52 }, { 0x73, 0x53 },
    { 0x74, 0x54 }, { 0x75, 0x55 }, { 0x76, 0x56 }, { 0x77, 0x57 },
    { 0x78, 0x58 }, { 0x79, 0x59 }, { 0x7a, 0x5a }, { 0x7b, 0x7b },
    { 0x7c, 0x7c }, { 0x7d, 0x7d }, { 0x7e, 0x7e }, { 0x7f, 0x7f },
    { 0x80, 0x80 }, { 0x81, 0x81 }, { 0x82, 0x82 }, { 0x83, 0x83 },
    { 0x84, 0x84 }, { 0x85, 0x85 }, { 0x86, 0x86 }, { 0x87, 0x87 },
    { 0x88, 0x88 }, { 0x89, 0x89 }, { 0x8a, 0x8a }, { 0x8b, 0x8b },
    { 0x8c, 0x8c }, { 0x8d, 0x8d }, { 0x8e, 0x8e }, { 0x8f, 0x8f },
    { 0x90, 0x90 }, { 0x91, 0x91 }, { 0x92, 0x92 }, { 0x93, 0x93 },
    { 0x94, 0x94 }, { 0x95, 0x95 }, { 0x96, 0x96 }, { 0x97, 0x97 },
    { 0x98, 0x98 }, { 0x99, 0x99 }, { 0x9a, 0x9a }, { 0x9b, 0x9b },
    { 0x9c, 0x9c }, { 0x9d, 0x9d }, { 0x9e, 0x9e }, { 0x9f, 0x9f },
    { 0xa0, 0xa0 }, { 0xa1, 0xa1 }, { 0xa2, 0xa2 }, { 0xa3, 0xa3 },
    { 0xa4, 0xa4 }, { 0xa5, 0xa5 }, { 0xa6, 0xa6 }, { 0xa7, 0xa7 },
    { 0xa8, 0xa8 }, { 0xa9, 0xa9 }, { 0xaa, 0xaa }, { 0xab, 0xab },
    { 0xac, 0xac }, { 0xad, 0xad }, { 0xae, 0xae }, { 0xaf, 0xaf },
    { 0xb0, 0xb0 }, { 0xb1, 0xb1 }, { 0xb2, 0xb2 }, { 0xb3, 0xb3 },
    { 0xb4, 0xb4 }, { 0xb5, 0xb5 }, { 0xb6, 0xb6 }, { 0xb7, 0xb7 },
    { 0xb8, 0xb8 }, { 0xb9, 0xb9 }, { 0xba, 0xba }, { 0xbb, 0xbb },
    { 0xbc, 0xbc }, { 0xbd, 0xbd }, { 0xbe, 0xbe }, { 0xbf, 0xbf },
    { 0xc0, 0xc0 }, { 0xc1, 0xc1 }, { 0xc2, 0xc2 }, { 0xc3, 0xc3 },
    { 0xc4, 0xc4 }, { 0xc5, 0xc5 }, { 0xc6, 0xc6 }, { 0xc7, 0xc7 },
    { 0xc8, 0xc8 }, { 0xc9, 0xc9 }, { 0xca, 0xca }, { 0xcb, 0xcb },
    { 0xcc, 0xcc }, { 0xcd, 0xcd }, { 0xce, 0xce }, { 0xcf, 0xcf },
    { 0xd0, 0xd0 }, { 0xd1, 0xd1 }, { 0xd2, 0xd2 }, { 0xd3, 0xd3 },
    { 0xd4, 0xd4 }, { 0xd5, 0xd5 }, { 0xd6, 0xd6 }, { 0xd7, 0xd7 },
    { 0xd8, 0xd8 }, { 0xd9, 0xd9 }, { 0xda, 0xda }, { 0xdb, 0xdb },
    { 0xdc, 0xdc }, { 0xdd, 0xdd }, { 0xde, 0xde }, { 0xdf, 0xdf },
    { 0xe0, 0xe0 }, { 0xe1, 0xe1 }, { 0xe2, 0xe2 }, { 0xe3, 0xe3 },
    { 0xe4, 0xe4 }, { 0xe5, 0xe5 }, { 0xe6, 0xe6 }, { 0xe7, 0xe7 },
    { 0xe8, 0xe8 }, { 0xe9, 0xe9 }, { 0xea, 0xea }, { 0xeb, 0xeb },
    { 0xec, 0xec }, { 0xed, 0xed }, { 0xee, 0xee }, { 0xef, 0xef },
    { 0xf0, 0xf0 }, { 0xf1, 0xf1 }, { 0xf2, 0xf2 }, { 0xf3, 0xf3 },
    { 0xf4, 0xf4 }, { 0xf5, 0xf5 }, { 0xf6, 0xf6 }, { 0xf7, 0xf7 },
    { 0xf8, 0xf8 }, { 0xf9, 0xf9 }, { 0xfa, 0xfa }, { 0xfb, 0xfb },
    { 0xfc, 0xfc }, { 0xfd, 0xfd }, { 0xfe, 0xfe }, { 0xff, 0xff }
};

int charstr_case_cmp(const char *a, const char *b)
{
    while (*a) {
        unsigned char ac = case_table[*(unsigned char *) a++].lower;
        unsigned char bc = case_table[*(unsigned char *) b++].lower;
        if (ac < bc)
            return -1;
        if (ac > bc)
            return 1;
    }
    if (*b)
        return -1;
    return 0;
}

int charstr_ncase_cmp(const char *a, const char *b, size_t n)
{
    for (; n && *a; n--) {
        unsigned char ac = case_table[*(unsigned char *) a++].lower;
        unsigned char bc = case_table[*(unsigned char *) b++].lower;
        if (ac < bc)
            return -1;
        if (ac > bc)
            return 1;
    }
    if (n && *b)
        return -1;
    return 0;
}

static const uint64_t char_class[256] = {
    CHARSTR_CONTROL,                                             /* '\000' */
    CHARSTR_CONTROL,                                             /* '\001' */
    CHARSTR_CONTROL,                                             /* '\002' */
    CHARSTR_CONTROL,                                             /* '\003' */
    CHARSTR_CONTROL,                                             /* '\004' */
    CHARSTR_CONTROL,                                             /* '\005' */
    CHARSTR_CONTROL,                                             /* '\006' */
    CHARSTR_CONTROL,                                             /* '\007' */
    CHARSTR_CONTROL | CHARSTR_LF,                                /* '\n' */
    CHARSTR_CONTROL | CHARSTR_HT,                                /* '\t' */
    CHARSTR_CONTROL | CHARSTR_FF,                                /* '\f' */
    CHARSTR_CONTROL | CHARSTR_CR,                                /* '\r' */
    CHARSTR_CONTROL,                                             /* '\014' */
    CHARSTR_CONTROL,                                             /* '\015' */
    CHARSTR_CONTROL,                                             /* '\016' */
    CHARSTR_CONTROL,                                             /* '\017' */
    CHARSTR_CONTROL,                                             /* '\020' */
    CHARSTR_CONTROL,                                             /* '\021' */
    CHARSTR_CONTROL,                                             /* '\022' */
    CHARSTR_CONTROL,                                             /* '\023' */
    CHARSTR_CONTROL,                                             /* '\024' */
    CHARSTR_CONTROL,                                             /* '\025' */
    CHARSTR_CONTROL,                                             /* '\026' */
    CHARSTR_CONTROL,                                             /* '\027' */
    CHARSTR_CONTROL,                                             /* '\030' */
    CHARSTR_CONTROL,                                             /* '\031' */
    CHARSTR_CONTROL,                                             /* '\032' */
    CHARSTR_CONTROL,                                             /* '\033' */
    CHARSTR_CONTROL,                                             /* '\034' */
    CHARSTR_CONTROL,                                             /* '\035' */
    CHARSTR_CONTROL,                                             /* '\036' */
    CHARSTR_CONTROL,                                             /* '\037' */
    CHARSTR_PRINT | CHARSTR_SPACE,                               /* ' ' */
    CHARSTR_PRINT,                                               /* '!' */
    CHARSTR_PRINT,                                               /* '"' */
    CHARSTR_PRINT,                                               /* '#' */
    CHARSTR_PRINT,                                               /* '$' */
    CHARSTR_PRINT,                                               /* '%' */
    CHARSTR_PRINT,                                               /* '&' */
    CHARSTR_PRINT,                                               /* '\'' */
    CHARSTR_PRINT,                                               /* '(' */
    CHARSTR_PRINT,                                               /* ')' */
    CHARSTR_PRINT,                                               /* '*' */
    CHARSTR_PRINT,                                               /* '+' */
    CHARSTR_PRINT,                                               /* ',' */
    CHARSTR_PRINT,                                               /* '-' */
    CHARSTR_PRINT,                                               /* '.' */
    CHARSTR_PRINT,                                               /* '/' */
    CHARSTR_PRINT | CHARSTR_DIGIT | CHARSTR_HEX | CHARSTR_OCTAL, /* '0' */
    CHARSTR_PRINT | CHARSTR_DIGIT | CHARSTR_HEX | CHARSTR_OCTAL, /* '1' */
    CHARSTR_PRINT | CHARSTR_DIGIT | CHARSTR_HEX | CHARSTR_OCTAL, /* '2' */
    CHARSTR_PRINT | CHARSTR_DIGIT | CHARSTR_HEX | CHARSTR_OCTAL, /* '3' */
    CHARSTR_PRINT | CHARSTR_DIGIT | CHARSTR_HEX | CHARSTR_OCTAL, /* '4' */
    CHARSTR_PRINT | CHARSTR_DIGIT | CHARSTR_HEX | CHARSTR_OCTAL, /* '5' */
    CHARSTR_PRINT | CHARSTR_DIGIT | CHARSTR_HEX | CHARSTR_OCTAL, /* '6' */
    CHARSTR_PRINT | CHARSTR_DIGIT | CHARSTR_HEX | CHARSTR_OCTAL, /* '7' */
    CHARSTR_PRINT | CHARSTR_DIGIT | CHARSTR_HEX,                 /* '8' */
    CHARSTR_PRINT | CHARSTR_DIGIT | CHARSTR_HEX,                 /* '9' */
    CHARSTR_PRINT,                                               /* ':' */
    CHARSTR_PRINT,                                               /* ';' */
    CHARSTR_PRINT,                                               /* '<' */
    CHARSTR_PRINT,                                               /* '=' */
    CHARSTR_PRINT,                                               /* '>' */
    CHARSTR_PRINT,                                               /* '?' */
    CHARSTR_PRINT,                                               /* '@' */
    CHARSTR_PRINT | CHARSTR_UPPER | CHARSTR_HEX,                 /* 'A' */
    CHARSTR_PRINT | CHARSTR_UPPER | CHARSTR_HEX,                 /* 'B' */
    CHARSTR_PRINT | CHARSTR_UPPER | CHARSTR_HEX,                 /* 'C' */
    CHARSTR_PRINT | CHARSTR_UPPER | CHARSTR_HEX,                 /* 'D' */
    CHARSTR_PRINT | CHARSTR_UPPER | CHARSTR_HEX,                 /* 'E' */
    CHARSTR_PRINT | CHARSTR_UPPER | CHARSTR_HEX,                 /* 'F' */
    CHARSTR_PRINT | CHARSTR_UPPER,                               /* 'G' */
    CHARSTR_PRINT | CHARSTR_UPPER,                               /* 'H' */
    CHARSTR_PRINT | CHARSTR_UPPER,                               /* 'I' */
    CHARSTR_PRINT | CHARSTR_UPPER,                               /* 'J' */
    CHARSTR_PRINT | CHARSTR_UPPER,                               /* 'K' */
    CHARSTR_PRINT | CHARSTR_UPPER,                               /* 'L' */
    CHARSTR_PRINT | CHARSTR_UPPER,                               /* 'M' */
    CHARSTR_PRINT | CHARSTR_UPPER,                               /* 'N' */
    CHARSTR_PRINT | CHARSTR_UPPER,                               /* 'O' */
    CHARSTR_PRINT | CHARSTR_UPPER,                               /* 'P' */
    CHARSTR_PRINT | CHARSTR_UPPER,                               /* 'Q' */
    CHARSTR_PRINT | CHARSTR_UPPER,                               /* 'R' */
    CHARSTR_PRINT | CHARSTR_UPPER,                               /* 'S' */
    CHARSTR_PRINT | CHARSTR_UPPER,                               /* 'T' */
    CHARSTR_PRINT | CHARSTR_UPPER,                               /* 'U' */
    CHARSTR_PRINT | CHARSTR_UPPER,                               /* 'V' */
    CHARSTR_PRINT | CHARSTR_UPPER,                               /* 'W' */
    CHARSTR_PRINT | CHARSTR_UPPER,                               /* 'X' */
    CHARSTR_PRINT | CHARSTR_UPPER,                               /* 'Y' */
    CHARSTR_PRINT | CHARSTR_UPPER,                               /* 'Z' */
    CHARSTR_PRINT,                                               /* '[' */
    CHARSTR_PRINT,                                               /* '\' */
    CHARSTR_PRINT,                                               /* ']' */
    CHARSTR_PRINT,                                               /* '^' */
    CHARSTR_PRINT | CHARSTR_UNDER,                               /* '_' */
    CHARSTR_PRINT,                                               /* '`' */
    CHARSTR_PRINT | CHARSTR_LOWER | CHARSTR_HEX,                 /* 'a' */
    CHARSTR_PRINT | CHARSTR_LOWER | CHARSTR_HEX,                 /* 'b' */
    CHARSTR_PRINT | CHARSTR_LOWER | CHARSTR_HEX,                 /* 'c' */
    CHARSTR_PRINT | CHARSTR_LOWER | CHARSTR_HEX,                 /* 'd' */
    CHARSTR_PRINT | CHARSTR_LOWER | CHARSTR_HEX,                 /* 'e' */
    CHARSTR_PRINT | CHARSTR_LOWER | CHARSTR_HEX,                 /* 'f' */
    CHARSTR_PRINT | CHARSTR_LOWER,                               /* 'g' */
    CHARSTR_PRINT | CHARSTR_LOWER,                               /* 'h' */
    CHARSTR_PRINT | CHARSTR_LOWER,                               /* 'i' */
    CHARSTR_PRINT | CHARSTR_LOWER,                               /* 'j' */
    CHARSTR_PRINT | CHARSTR_LOWER,                               /* 'k' */
    CHARSTR_PRINT | CHARSTR_LOWER,                               /* 'l' */
    CHARSTR_PRINT | CHARSTR_LOWER,                               /* 'm' */
    CHARSTR_PRINT | CHARSTR_LOWER,                               /* 'n' */
    CHARSTR_PRINT | CHARSTR_LOWER,                               /* 'o' */
    CHARSTR_PRINT | CHARSTR_LOWER,                               /* 'p' */
    CHARSTR_PRINT | CHARSTR_LOWER,                               /* 'q' */
    CHARSTR_PRINT | CHARSTR_LOWER,                               /* 'r' */
    CHARSTR_PRINT | CHARSTR_LOWER,                               /* 's' */
    CHARSTR_PRINT | CHARSTR_LOWER,                               /* 't' */
    CHARSTR_PRINT | CHARSTR_LOWER,                               /* 'u' */
    CHARSTR_PRINT | CHARSTR_LOWER,                               /* 'v' */
    CHARSTR_PRINT | CHARSTR_LOWER,                               /* 'w' */
    CHARSTR_PRINT | CHARSTR_LOWER,                               /* 'x' */
    CHARSTR_PRINT | CHARSTR_LOWER,                               /* 'y' */
    CHARSTR_PRINT | CHARSTR_LOWER,                               /* 'z' */
    CHARSTR_PRINT,                                               /* '{' */
    CHARSTR_PRINT,                                               /* '|' */
    CHARSTR_PRINT,                                               /* '}' */
    CHARSTR_PRINT,                                               /* '~' */
    CHARSTR_CONTROL                                              /* '\177' */
};

uint64_t charstr_char_class(char c)
{
    return char_class[(unsigned char) c];
}

char charstr_lcase_char(char c)
{
    return case_table[(unsigned char) c].lower;
}

char charstr_ucase_char(char c)
{
    return case_table[(unsigned char) c].upper;
}

const char *charstr_skip_prefix(const char *s, const char *prefix)
{
    while (*prefix)
        if (*s++ != *prefix++)
            return NULL;
    return s;
}

const char *charstr_case_starts_with(const char *s, const char *prefix)
{
    return charstr_skip_prefix(s, prefix);
}

const char *charstr_case_skip_prefix(const char *s, const char *prefix)
{
    while (*prefix)
        if (charstr_lcase_char(*s++) != charstr_lcase_char(*prefix++))
            return NULL;
    return s;
}

const char *charstr_ncase_starts_with(const char *s, const char *prefix)
{
    return charstr_case_skip_prefix(s, prefix);
}

bool charstr_ends_with(const char *s, const char *suffix)
{
    size_t len = strlen(s);
    size_t suffix_len = strlen(suffix);

    if (len < suffix_len)
        return false;

    return !strcmp(s + len - suffix_len, suffix);
}

unsigned charstr_digit_value(char c)
{
    switch (c) {
        case '0':
            return 0;
        case '1':
            return 1;
        case '2':
            return 2;
        case '3':
            return 3;
        case '4':
            return 4;
        case '5':
            return 5;
        case '6':
            return 6;
        case '7':
            return 7;
        case '8':
            return 8;
        case '9':
            return 9;
        case 'A':
        case 'a':
            return 10;
        case 'B':
        case 'b':
            return 11;
        case 'C':
        case 'c':
            return 12;
        case 'D':
        case 'd':
            return 13;
        case 'E':
        case 'e':
            return 14;
        case 'F':
        case 'f':
            return 15;
        default:
            return -1;
    }
}

static unsigned natural_base(const char **s, const char *end)
{
    const char *sp = *s;
    if (sp == end || *sp != '0')
        return 10;
    sp++;
    if (sp == end || !(*sp == 'x' || *sp == 'X'))
        return 8;
    *s = sp + 1;
    return 16;
}

static ssize_t to_unsigned(const char *buffer, const char *end, unsigned base,
                           uint64_t *value, bool *overflow)
{
    const char *digits = buffer;
    if (base == 0)
        base = natural_base(&digits, end);
    if (base < 2 || base > 16) {
        errno = EINVAL;
        return -1;
    }
    *overflow = false;
    uint64_t n = 0;
    uint64_t limit = (uint64_t) -1 / base;
    const char *dp = digits;
    for (; dp != end; dp++) {
        unsigned d = charstr_digit_value(*dp);
        if (d == -1U || d >= base)
            break;
        uint64_t old_n = n;
        n = base * n + d;
        if (old_n > limit || n < old_n)
            *overflow = true;
    }
    if (dp == digits) {
        errno = EILSEQ;
        return -1;
    }
    *value = n;
    return dp - buffer;
}

ssize_t charstr_to_unsigned(const char *buffer, size_t size, unsigned base,
                            uint64_t *value)
{
    const char *end = buffer + size;
    bool overflow;
    ssize_t count = to_unsigned(buffer, end, base, value, &overflow);
    if (count >= 0 && overflow)
        errno = ERANGE;
    return count;
}

static int64_t parse_positive(const char *s, const char *end, unsigned base,
                              int64_t *value)
{
    uint64_t u;
    bool overflow;
    ssize_t count = to_unsigned(s, end, base, &u, &overflow);
    if (count >= 0) {
        if (overflow || (int64_t) u < 0)
            errno = ERANGE;
        *value = u;
    }
    return count;
}

static int64_t parse_negative(const char *s, const char *end, unsigned base,
                              int64_t *value)
{
    uint64_t u;
    bool overflow;
    ssize_t count = to_unsigned(s, end, base, &u, &overflow);
    if (count >= 0) {
        if (overflow || (int64_t) -u > 0)
            errno = ERANGE;
        *value = -u;
    }
    return count;
}

ssize_t charstr_to_integer(const char *buffer, size_t size, unsigned base,
                           int64_t *value)
{
    if (!size) {
        errno = EILSEQ;
        return 0;
    }
    const char *end = buffer + size;
    ssize_t count;
    switch (*buffer) {
        case '+':
            count = parse_positive(buffer + 1, end, base, value);
            if (count < 0)
                return count;
            return count + 1;
        case '-':
            count = parse_negative(buffer + 1, end, base, value);
            if (count < 0)
                return count;
            return count + 1;
        default:
            return parse_positive(buffer, end, base, value);
    }
}

char *charstr_dupstr(const char *s)
{
    if (!s)
        return NULL;
    return charstr_dupsubstr(s, s + strlen(s));
}

char *charstr_dupsubstr(const char *s, const char *end)
{
    if (!s || !end)
        return charstr_dupstr(s);
    size_t length = end - s;
    char *dup = fsalloc(length + 1);
    memcpy(dup, s, length);
    dup[length] = '\0';
    return dup;
}

char *charstr_lcase_str(char *s)
{
    char *p;
    for (p = s; *p; p++)
        *p = charstr_lcase_char(*p);
    return s;
}

char *charstr_lcase_substr(char *s, const char *end)
{
    char *p;
    for (p = s; p != end; p++)
        *p = charstr_lcase_char(*p);
    return s;
}

char *charstr_ucase_str(char *s)
{
    char *p;
    for (p = s; *p; p++)
        *p = charstr_ucase_char(*p);
    return s;
}

char *charstr_ucase_substr(char *s, const char *end)
{
    char *p;
    for (p = s; p != end; p++)
        *p = charstr_ucase_char(*p);
    return s;
}

list_t *charstr_split(const char *s, char delim, unsigned max_split)
{
    list_t *list = make_list();
    while (max_split--) {
        const char *p = strchr(s, delim);
        if (!p)
            break;
        list_append(list, charstr_dupsubstr(s, p));
        s = p + 1;
    }
    list_append(list, charstr_dupstr(s));
    return list;
}

unsigned charstr_split_into_array(const char *s, char delim, char **array,
                                  unsigned max_split)
{
    unsigned i = 0;
    while (i < max_split) {
        const char *p = strchr(s, delim);
        if (!p)
            break;
        array[i++] = charstr_dupsubstr(s, p);
        s = p + 1;
    }
    array[i] = charstr_dupstr(s);
    return i;
}

list_t *charstr_split_atoms(const char *s)
{
    list_t *list = make_list();
    for (;;) {
        for (;; s++)
            if (!*s)
                return list;
            else if (!(charstr_char_class(*s) & CHARSTR_WHITESPACE))
                break;
        const char *p = s + 1;
        while (*p && !(charstr_char_class(*p) & CHARSTR_WHITESPACE))
            p++;
        list_append(list, charstr_dupsubstr(s, p));
        s = p;
    }
}

list_t *charstr_split_str(const char *s, const char *delim, unsigned max_split)
{
    assert(*delim);
    list_t *list = make_list();
    size_t skip = strlen(delim);
    while (max_split--) {
        const char *p = strstr(s, delim);
        if (!p)
            break;
        list_append(list, charstr_dupsubstr(s, p));
        s = p + skip;
    }
    list_append(list, charstr_dupstr(s));
    return list;
}

char *charstr_strip(const char *s)
{
    if (!s)
        return NULL;
    while (*s && charstr_char_class(*s) & CHARSTR_WHITESPACE)
        s++;
    const char *end = s + strlen(s);
    while (end > s && charstr_char_class(end[-1]) & CHARSTR_WHITESPACE)
        end--;
    return charstr_dupsubstr(s, end);
}

char *charstr_join(const char *joiner, list_t *strings)
{
    list_elem_t *e = list_get_first(strings);
    byte_array_t *array = make_byte_array(SIZE_MAX);
    if (e) {
        byte_array_append_string(array, list_elem_get_value(e));
        for (e = list_next(e); e; e = list_next(e)) {
            byte_array_append_string(array, joiner);
            byte_array_append_string(array, list_elem_get_value(e));
        }
    }
    char *result = charstr_dupstr(byte_array_data(array));
    destroy_byte_array(array);
    return result;
}

static bool valid_unicode(int codepoint, int lowest)
{
    return codepoint >= lowest &&
        (codepoint <= 0xd7ff || (codepoint >= 0xe000 && codepoint <= 0x10ffff));
}

const char *charstr_decode_utf8_codepoint(const char *s, const char *end,
                                          int *codepoint)
{
    int cp;
    if (!s || s == end)
        return NULL;
    if (!(*s & 0x80))
        cp = *s++ & 0x7f;
    else if (!(*s & 0x40))
        return NULL;
    else {
        int lowest;
        if (!(*s & 0x20)) {
            cp = *s++ & 0x1f;
            lowest = 0x80;
        } else {
            if (!(*s & 0x10)) {
                cp = *s++ & 0x0f;
                lowest = 0x800;
            } else {
                if (*s & 0x08)
                    return NULL;
                cp = *s++ & 0x07;
                lowest = 0x10000;
                if (s == end || (*s & 0xc0) != 0x80)
                    return NULL;
                cp = cp << 6 | *s++ & 0x3f;
            }
            if (s == end || (*s & 0xc0) != 0x80)
                return NULL;
            cp = cp << 6 | *s++ & 0x3f;
        }
        if (s == end || (*s & 0xc0) != 0x80)
            return NULL;
        cp = cp << 6 | *s++ & 0x3f;
        if (!valid_unicode(cp, lowest))
            return NULL;
    }
    if (codepoint)
        *codepoint = cp;
    return s;
}

bool charstr_valid_utf8_bounded(const char *s, const char *end)
{
    while (s && s != end)
        s = charstr_decode_utf8_codepoint(s, end, NULL);
    return s != NULL;
}

bool charstr_valid_utf8(const char *s)
{
    while (s && *s)
        s = charstr_decode_utf8_codepoint(s, NULL, NULL);
    return s != NULL;
}

char *charstr_sanitize_utf8(const char *s)
{
    const char REPLACEMENT_CHARACTER[3] = { 0xef, 0xbf, 0xbd };
    size_t bad_bytes = 0;
    const char *t = s;
    while (*t) {
        const char *u = charstr_decode_utf8_codepoint(t, NULL, NULL);
        if (u)
            t = u;
        else {
            t++;
            bad_bytes++;
        }
    }
    size_t sane_length = t - s + bad_bytes * (sizeof REPLACEMENT_CHARACTER - 1);
    char *sane = fsalloc(sane_length + 1);
    char *q = sane;
    while (*s) {
        const char *u = charstr_decode_utf8_codepoint(s, NULL, NULL);
        if (u)
            while (s != u)
                *q++ = *s++;
        else {
            s++;
            memcpy(q, REPLACEMENT_CHARACTER, sizeof REPLACEMENT_CHARACTER);
            q += sizeof REPLACEMENT_CHARACTER;
        }
    }
    *q = '\0';
    return sane;
}

char *charstr_encode_utf8_codepoint(int codepoint, char *s, const char *end)
{
    if (!s || s == end)
        return NULL;
    if (codepoint >= 0x80) {
        if (codepoint >= 0x800) {
            if (codepoint >= 0x10000) {
                *s++ = 0xf0 | codepoint >> 18;
                if (s == end)
                    return NULL;
                *s++ = 0x80 | codepoint >> 12 & 0x3f;
            } else
                *s++ = 0xe0 | codepoint >> 12;
            if (s == end)
                return NULL;
            *s++ = 0x80 | codepoint >> 6 & 0x3f;
        } else
            *s++ = 0xc0 | codepoint >> 6;
        if (s == end)
            return NULL;
        *s++ = 0x80 | codepoint & 0x3f;
    } else
        *s++ = codepoint;
    return s;
}

struct charstr_url_encoder {
    char table[256][4];
};

static charstr_url_encoder_t url_encoder = {
    .table = {
        "%00", "%01", "%02", "%03", "%04", "%05", "%06", "%07",
        "%08", "%09", "%0A", "%0B", "%0C", "%0D", "%0E", "%0F",
        "%10", "%11", "%12", "%13", "%14", "%15", "%16", "%17",
        "%18", "%19", "%1A", "%1B", "%1C", "%1D", "%1E", "%1F",
        "+",   "%21", "\"",  "%23", "%24", "%25", "%26", "%27",
        "%28", "%29", "%2A", "%2B", "%2C", "-",   ".",   "/",
        "0",   "1",   "2",    "3",   "4",  "5",   "6",   "7",
        "8",   "9",   "%3A",  "%3B", "<",  "%3D", ">",   "%3F",
        "%40", "A",   "B",    "C",   "D",  "E",   "F",   "G",
        "H",   "I",   "J",    "K",   "L",  "M",   "N",   "O",
        "P",   "Q",   "R",    "S",   "T",  "U",   "V",   "W",
        "X",   "Y",   "Z",    "%5B", "\\", "%5D", "^",   "_",
        "`",   "a",   "b",    "c",   "d",  "e",   "f",   "g",
        "h",   "i",   "j",    "k",   "l",  "m",   "n",   "o",
        "p",   "q",   "r",    "s",   "t",  "u",   "v",   "w",
        "x",   "y",   "z",    "{",   "|",  "}",   "~",   "%7F",
        "%80", "%81", "%82", "%83", "%84", "%85", "%86", "%87",
        "%88", "%89", "%8A", "%8B", "%8C", "%8D", "%8E", "%8F",
        "%90", "%91", "%92", "%93", "%94", "%95", "%96", "%97",
        "%98", "%99", "%9A", "%9B", "%9C", "%9D", "%9E", "%9F",
        "%A0", "%A1", "%A2", "%A3", "%A4", "%A5", "%A6", "%A7",
        "%A8", "%A9", "%AA", "%AB", "%AC", "%AD", "%AE", "%AF",
        "%B0", "%B1", "%B2", "%B3", "%B4", "%B5", "%B6", "%B7",
        "%B8", "%B9", "%BA", "%BB", "%BC", "%BD", "%BE", "%BF",
        "%C0", "%C1", "%C2", "%C3", "%C4", "%C5", "%C6", "%C7",
        "%C8", "%C9", "%CA", "%CB", "%CC", "%CD", "%CE", "%CF",
        "%D0", "%D1", "%D2", "%D3", "%D4", "%D5", "%D6", "%D7",
        "%D8", "%D9", "%DA", "%DB", "%DC", "%DD", "%DE", "%DF",
        "%E0", "%E1", "%E2", "%E3", "%E4", "%E5", "%E6", "%E7",
        "%E8", "%E9", "%EA", "%EB", "%EC", "%ED", "%EE", "%EF",
        "%F0", "%F1", "%F2", "%F3", "%F4", "%F5", "%F6", "%F7",
        "%F8", "%F9", "%FA", "%FB", "%FC", "%FD", "%FE", "%FF",
    },
};

charstr_url_encoder_t *charstr_create_url_encoder(const char *reserve,
                                                  const char *unreserve)
{
    charstr_url_encoder_t *encoder = fsalloc(sizeof *encoder);
    memcpy(encoder, &url_encoder, sizeof *encoder);
    int i;
    const char *p;
    for (p = reserve; *p; p++) {
        i = (uint8_t) *p;
        snprintf(encoder->table[i], sizeof encoder->table[i], "%%%02X", i);
    }
    for (p = unreserve; *p; p++) {
        i = (uint8_t) *p;
        snprintf(encoder->table[i], sizeof encoder->table[i], "%c", i);
    }
    return encoder;
}

void charstr_destroy_url_encoder(charstr_url_encoder_t *encoder)
{
    fsfree(encoder);
}

const char *charstr_url_custom_encode_byte(charstr_url_encoder_t *encoder,
                                           uint8_t byte)
{
    return encoder->table[byte];
}

const char *charstr_url_encode_byte(uint8_t byte)
{
    return charstr_url_custom_encode_byte(&url_encoder, byte);
}

static size_t url_encoding_width(charstr_url_encoder_t *encoder, uint8_t byte)
{
    switch (charstr_url_custom_encode_byte(encoder, byte)[0]) {
        case '%':
            return 3;
        default:
            return 1;
    }
}

char *charstr_url_custom_encode(charstr_url_encoder_t *encoder,
                                const char *string)
{
    size_t length = 0;
    const char *sp;
    for (sp = string; *sp; sp++)
        length += url_encoding_width(encoder, *sp);
    char *encoding = fsalloc(length + 1);
    if (!encoding)
        return NULL;
    const char *urlp;
    char *ep = encoding;
    for (sp = string; *sp; sp++)
        for (urlp = charstr_url_custom_encode_byte(encoder, *sp); *urlp;
             *ep++ = *urlp++)
            ;
    *ep = '\0';
    return encoding;
}

char *charstr_url_encode(const char *string)
{
    return charstr_url_custom_encode(&url_encoder, string);
}

char *charstr_url_decode(const char *encoding, bool plus_is_space, size_t *size)
{
    const char *ep = encoding;
    size_t length;
    for (length = 0; *ep; length++)
        if (*ep++ == '%' &&
            (!(charstr_char_class(*ep++) & CHARSTR_HEX) ||
             !(charstr_char_class(*ep++) & CHARSTR_HEX))) {
            errno = EILSEQ;
            return NULL;
        }
    char *decoding = fsalloc(length + 1);
    if (!decoding)
        return NULL;
    char *dp = decoding;
    ep = encoding;
    for (;;)
        switch (*ep) {
            case '\0':
                *dp = '\0';
                if (size)
                    *size = length;
                return decoding;
            case '+':
                *dp++ = plus_is_space ? ' ' : '+';
                ep++;
                break;
            case '%':
                ep++;
                *dp = charstr_digit_value(*ep++) * 16;
                *dp++ += charstr_digit_value(*ep++);
                break;
            default:
                *dp++ = *ep++;
        }
}

char *charstr_vprintf(const char *format, va_list ap)
{
    va_list copy;
    va_copy(copy, ap);
    ssize_t length = vsnprintf(NULL, 0, format, copy);
    va_end(copy);
    if (length < 0) {
        errno = EILSEQ;
        return NULL;
    }
    char *buffer = fsalloc(length + 1);
    if (buffer)
        vsprintf(buffer, format, ap);
    return buffer;
}

char *charstr_printf(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    char *buffer = charstr_vprintf(format, ap);
    va_end(ap);
    return buffer;
}
