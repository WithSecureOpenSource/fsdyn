#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fsdyn/charstr.h>

static bool test_decode_utf8_codepoint(void)
{
    const char *text = "h±ℂ態";
    const char *s;
    const char *end = strchr(text, 0);
    int cp;
    s = charstr_decode_utf8_codepoint(text, end, &cp);
    if (!s) {
        fprintf(stderr, "h -> NULL\n");
        return false;
    }
    if (cp != 'h') {
        fprintf(stderr, "h -> %d\n", cp);
        return false;
    }
    s = charstr_decode_utf8_codepoint(s, end, &cp);
    if (!s) {
        fprintf(stderr, "± -> NULL\n");
        return false;
    }
    if (cp != 0xb1) {
        fprintf(stderr, "± -> %d\n", cp);
        return false;
    }
    s = charstr_decode_utf8_codepoint(s, end, &cp);
    if (!s) {
        fprintf(stderr, "ℂ -> NULL\n");
        return false;
    }
    if (cp != 0x2102) {
        fprintf(stderr, "ℂ -> %d\n", cp);
        return false;
    }
    s = charstr_decode_utf8_codepoint(s, end, &cp);
    if (!s) {
        fprintf(stderr, "態 -> NULL\n");
        return false;
    }
    if (cp != 0x614b) {
        fprintf(stderr, "態 -> %d\n", cp);
        return false;
    }
    if (s != end) {
        fprintf(stderr, "Bad end\n");
        return false;
    }
    s = charstr_decode_utf8_codepoint(text, NULL, NULL);
    if (!s) {
        fprintf(stderr, "h --> NULL\n");
        return false;
    }
    s = charstr_decode_utf8_codepoint(s, NULL, NULL);
    if (!s) {
        fprintf(stderr, "± --> NULL\n");
        return false;
    }
    s = charstr_decode_utf8_codepoint(s, NULL, NULL);
    if (!s) {
        fprintf(stderr, "ℂ --> NULL\n");
        return false;
    }
    s = charstr_decode_utf8_codepoint(s, NULL, NULL);
    if (!s) {
        fprintf(stderr, "態 --> NULL\n");
        return false;
    }
    if (*s) {
        fprintf(stderr, "Not NUL-terminated\n");
        return false;
    }
    return true;
}

static bool test_ends_with(void)
{
    const char *text = "ℕ⊆ℕ₀";
    if (!charstr_ends_with(text, "ℕ⊆ℕ₀"))
        return false;
    if (charstr_ends_with(text, "ℕ⊆ℕ"))
        return false;
    if (!charstr_ends_with(text, "⊆ℕ₀"))
        return false;
    if (charstr_ends_with(text, "ℕ⊆"))
        return false;
    if (charstr_ends_with(text, "⊆ℕ"))
        return false;
    if (!charstr_ends_with(text, "ℕ₀"))
        return false;
    if (!charstr_ends_with(text, "₀"))
        return false;
    return true;
}

static const char *nfc_string = "Öiset häiriöt — šokkiko?";
static const char *mixed_string = "Öiset häiriöt — šokkiko?";
static const char *nfd_string = "Öiset häiriöt — šokkiko?";

static bool test_canonical_nfc(void)
{
    if (!charstr_unicode_canonically_composed(nfc_string, NULL))
        return false;
    if (charstr_unicode_canonically_composed(mixed_string, NULL))
        return false;
    if (charstr_unicode_canonically_composed(nfd_string, NULL))
        return false;
    return true;
}

static bool test_canonical_nfd(void)
{
    if (charstr_unicode_canonically_decomposed(nfc_string, NULL))
        return false;
    if (charstr_unicode_canonically_decomposed(mixed_string, NULL))
        return false;
    if (!charstr_unicode_canonically_decomposed(nfd_string, NULL))
        return false;
    return true;
}

static bool test_canonical_decomposition(void)
{
    char output[1000];
    const char *output_end = output + sizeof output;
    if (!charstr_unicode_decompose(nfc_string, NULL, output, output_end))
        return false;
    if (strcmp(nfd_string, output))
        return false;
    if (!charstr_unicode_decompose(mixed_string, NULL, output, output_end))
        return false;
    if (strcmp(nfd_string, output))
        return false;
    if (!charstr_unicode_decompose(nfd_string, NULL, output, output_end))
        return false;
    if (strcmp(nfd_string, output))
        return false;
    return true;
}

static bool test_canonical_recomposition(void)
{
    char output[1000];
    const char *output_end = output + sizeof output;
    if (!charstr_unicode_recompose(nfd_string, NULL, output, output_end))
        return false;
    if (strcmp(nfc_string, output))
        return false;
    return true;
}

static bool test_sanitization(void)
{
    const char *bad_utf8 =
        "¿Qué ha pasado? "
        "H\xe4n sai b\xe4n\xe4t tytt\xf6yst\xe4v\xe4lt\xe4\xe4n";
    const char *expected = "¿Qué ha pasado? "
                           "H�n sai b�n�t tytt�yst�v�lt��n";
    char *sanitized = charstr_sanitize_utf8(bad_utf8);
    bool result = !strcmp(sanitized, expected);
    fsfree(sanitized);
    return result;
}

static bool test_url_encoding(void)
{
    const char *source = "a ? b : c";
    char *encoding = charstr_url_encode(source);
    bool result = !strcmp(encoding, "a+%3F+b+%3A+c");
    if (!result) {
        fsfree(encoding);
        return false;
    }
    char *decoding = charstr_url_decode(encoding, true, NULL);
    result = !strcmp(source, decoding);
    fsfree(decoding);
    fsfree(encoding);
    if (!result)
        return false;
    charstr_url_encoder_t *encoder = charstr_create_url_encoder(" b", ":");
    encoding = charstr_url_custom_encode(encoder, source);
    result = !strcmp(encoding, "a%20%3F%20%62%20:%20c");
    if (!result) {
        fsfree(encoding);
        charstr_destroy_url_encoder(encoder);
        return false;
    }
    decoding = charstr_url_decode(encoding, true, NULL);
    result = !strcmp(source, decoding);
    fsfree(decoding);
    fsfree(encoding);
    charstr_destroy_url_encoder(encoder);
    return result;
}

static bool test_upper_lower(void)
{
    static const char *upcase_string = "ÖISET HÄIRIÖT — ŠOKKIKO?";
    static const char *downcase_string = "öiset häiriöt — šokkiko?";
    const char *p = upcase_string;
    const char *q = downcase_string;
    while (*p && *q) {
        int up, down;
        p = charstr_decode_utf8_codepoint(p, NULL, &up);
        q = charstr_decode_utf8_codepoint(q, NULL, &down);
        if (down != charstr_naive_lcase_unicode(up))
            return false;
        if (up != charstr_naive_ucase_unicode(down))
            return false;
    }
    return !*p && !*q;
}

static bool test_parse_digits_inval(void)
{
    uint64_t u;
    const char *_101 = "101";
    errno = 0;
    u = charstr_parse_digits(_101, NULL, 0);
    if (u || errno != EINVAL) {
        fprintf(stderr,
                "charstr_parse_digits(_101, NULL, 0) => (%llu, %d)\n",
                (unsigned long long) u, errno);
        return false;
    }
    errno = 0;
    u = charstr_parse_digits(_101, NULL, 1);
    if (u || errno != EINVAL) {
        fprintf(stderr,
                "charstr_parse_digits(_101, NULL, 1) => (%llu, %d)\n",
                (unsigned long long) u, errno);
        return false;
    }
    errno = 0;
    u = charstr_parse_digits(_101, NULL, 17);
    if (u || errno != EINVAL) {
        fprintf(stderr,
                "charstr_parse_digits(_101, NULL, 17) => (%llu, %d)\n",
                (unsigned long long) u, errno);
        return false;
    }
    return true;
}

static bool test_parse_digits_unlimited(void)
{
    uint64_t u;
    const char *_101 = "101";
    errno = 0;
    u = charstr_parse_digits(_101, NULL, 2);
    if (u != 5|| errno) {
        fprintf(stderr,
                "charstr_parse_digits(_101, NULL, 2) => (%llu, %d)\n",
                (unsigned long long) u, errno);
        return false;
    }
    u = charstr_parse_digits(_101, NULL, 5);
    if (u != 26|| errno) {
        fprintf(stderr,
                "charstr_parse_digits(_101, NULL, 5) => (%llu, %d)\n",
                (unsigned long long) u, errno);
        return false;
    }
    u = charstr_parse_digits(_101, NULL, 10);
    if (u != 101|| errno) {
        fprintf(stderr,
                "charstr_parse_digits(_101, NULL, 10) => (%llu, %d)\n",
                (unsigned long long) u, errno);
        return false;
    }
    u = charstr_parse_digits(_101, NULL, 16);
    if (u != 257|| errno) {
        fprintf(stderr,
                "charstr_parse_digits(_101, NULL, 16) => (%llu, %d)\n",
                (unsigned long long) u, errno);
        return false;
    }
    u = charstr_parse_digits("", NULL, 10);
    if (u || errno != EILSEQ) {
        fprintf(stderr,
                "charstr_parse_digits(\"\", NULL, 10) => (%llu, %d)\n",
                (unsigned long long) u, errno);
        return false;
    }
    errno = 0;
    u = charstr_parse_digits("A", NULL, 10);
    if (u || errno != EILSEQ) {
        fprintf(stderr,
                "charstr_parse_digits(\"\", NULL, 10) => (%llu, %d)\n",
                (unsigned long long) u, errno);
        return false;
    }
    return true;
}

static bool test_parse_digits_limited(void)
{
    uint64_t u;
    const char *_101 = "101\n";
    const char *A = "A\n";
    const char *end;
    errno = 0;
    end = _101 + 5;
    u = charstr_parse_digits(_101, &end, 10);
    if (u != 101|| errno || end != _101 + 3) {
        fprintf(stderr,
                "charstr_parse_digits(_101, ... + 5, 10) => (%llu, %d, %d)\n",
                (unsigned long long) u, errno, (int) (end - _101));
        return false;
    }
    end = _101 + 3;
    u = charstr_parse_digits(_101, &end, 10);
    if (u != 101|| errno || end != _101 + 3) {
        fprintf(stderr,
                "charstr_parse_digits(_101, ... + 3, 10) => (%llu, %d, %d)\n",
                (unsigned long long) u, errno, (int) (end - _101));
        return false;
    }
    end = _101 + 2;
    u = charstr_parse_digits(_101, &end, 10);
    if (u != 10 || errno || end != _101 + 2) {
        fprintf(stderr,
                "charstr_parse_digits(_101, ... + 2, 10) => (%llu, %d, %d)\n",
                (unsigned long long) u, errno, (int) (end - _101));
        return false;
    }
    end = _101;
    u = charstr_parse_digits(_101, &end, 10);
    if (u || errno != EILSEQ) {
        fprintf(stderr,
                "charstr_parse_digits(_101, ... + 0, 10) => (%llu, %d)\n",
                (unsigned long long) u, errno);
        return false;
    }
    errno = 0;
    end = A + 1;
    u = charstr_parse_digits(A, &end, 10);
    if (u || errno != EILSEQ) {
        fprintf(stderr,
                "charstr_parse_digits(A, ... + 1, 10) => (%llu, %d)\n",
                (unsigned long long) u, errno);
        return false;
    }
    return true;
}

static bool test_parse_digits_range(void)
{
    uint64_t u;
    errno = 0;
    u = charstr_parse_digits("0000000000000000", NULL, 16);
    if (u || errno) {
        fprintf(stderr,
                "charstr_parse_digits(\"0000000000000000\", NULL, 16) => "
                "(0x%llx, %d)\n",
                (unsigned long long) u, errno);
        return false;
    }
    u = charstr_parse_digits("8000000000000000", NULL, 16);
    if (u != 0x8000000000000000 || errno) {
        fprintf(stderr,
                "charstr_parse_digits(\"8000000000000000\", NULL, 16) => "
                "(0x%llx, %d)\n",
                (unsigned long long) u, errno);
        return false;
    }
    u = charstr_parse_digits("ffffffffffffffff", NULL, 16);
    if (u != 0xffffffffffffffff || errno) {
        fprintf(stderr,
                "charstr_parse_digits(\"ffffffffffffffff\", NULL, 16) => "
                "(0x%llx, %d)\n",
                (unsigned long long) u, errno);
        return false;
    }
    u = charstr_parse_digits("18446744073709551615", NULL, 10);
    if (u != 18446744073709551615ULL || errno) {
        fprintf(stderr,
                "charstr_parse_digits(\"18446744073709551615\", NULL, 10) => "
                "(%lld, %d)\n",
                (unsigned long long) u, errno);
        return false;
    }
    u = charstr_parse_digits("ffffffffffffffffc", NULL, 16);
    if (u != 0xfffffffffffffffc || errno != ERANGE) {
        fprintf(stderr,
                "charstr_parse_digits(\"ffffffffffffffffc\", NULL, 16) => "
                "(0x%llx, %d)\n",
                (unsigned long long) u, errno);
        return false;
    }
    return true;
}

static bool test_parse_digits(void)
{
    return test_parse_digits_inval() && 
        test_parse_digits_unlimited() &&
        test_parse_digits_limited() &&
        test_parse_digits_range();
}

static bool test_parse_signed_inval(void)
{
    int64_t v;
    const char *_101 = "101";
    errno = 0;
    v = charstr_parse_signed(_101, NULL, 1);
    if (v || errno != EINVAL) {
        fprintf(stderr,
                "charstr_parse_signed(_101, NULL, 1) => (%llu, %d)\n",
                (unsigned long long) v, errno);
        return false;
    }
    errno = 0;
    v = charstr_parse_signed(_101, NULL, 17);
    if (v || errno != EINVAL) {
        fprintf(stderr,
                "charstr_parse_signed(_101, NULL, 17) => (%llu, %d)\n",
                (unsigned long long) v, errno);
        return false;
    }
    return true;
}

static bool test_parse_signed_unlimited(void)
{
    int64_t v;
    errno = 0;
    v = charstr_parse_signed(" 12", NULL, 2);
    if (v != 1 || errno) {
        fprintf(stderr,
                "charstr_parse_signed(\" 12\", NULL, 2) => (%llu, %d)\n",
                (unsigned long long) v, errno);
        return false;
    }
    v = charstr_parse_signed(" \n-12", NULL, 8);
    if (v != -10 || errno) {
        fprintf(stderr,
                "charstr_parse_signed(\" \\n-12\", NULL, 8) => (%llu, %d)\n",
                (unsigned long long) v, errno);
        return false;
    }
    v = charstr_parse_signed("-12", NULL, 0);
    if (v != -12 || errno) {
        fprintf(stderr,
                "charstr_parse_signed(\"-12\", NULL, 0) => (%llu, %d)\n",
                (unsigned long long) v, errno);
        return false;
    }
    v = charstr_parse_signed("\t+0x12", NULL, 0);
    if (v != 18 || errno) {
        fprintf(stderr,
                "charstr_parse_signed(\"\\t+0x12\", NULL, 0) => (%llu, %d)\n",
                (unsigned long long) v, errno);
        return false;
    }
    v = charstr_parse_signed("\t+012", NULL, 0);
    if (v != 10 || errno) {
        fprintf(stderr,
                "charstr_parse_signed(\"\\t+012\", NULL, 0) => (%llu, %d)\n",
                (unsigned long long) v, errno);
        return false;
    }
    v = charstr_parse_signed("0xQ", NULL, 0);
    if (v || errno != EILSEQ) {
        fprintf(stderr,
                "charstr_parse_signed(\"0xQ\", NULL, 0) => (%llu, %d)\n",
                (unsigned long long) v, errno);
        return false;
    }
    return true;
}

static bool test_parse_signed_limited(void)
{
    int64_t v;
    errno = 0;
    const char *_0x101 = " 0x101\n";
    const char *end;
    end = _0x101 + 5;
    v = charstr_parse_signed(_0x101, &end, 2);
    if (v != 0 || errno || end != _0x101 + 2) {
        fprintf(stderr,
                "charstr_parse_signed(_0x101, ... + 5, 2) => (%llu, %d, %d)\n",
                (unsigned long long) v, errno, (int) (end - _0x101));
        return false;
    }
    end = _0x101 + 5;
    v = charstr_parse_signed(_0x101, &end, 0);
    if (v != 16 || errno || end != _0x101 + 5) {
        fprintf(stderr,
                "charstr_parse_signed(_0x101, ... + 5, 16) "
                "=> (%llu, %d, %d)\n",
                (unsigned long long) v, errno, (int) (end - _0x101));
        return false;
    }
    end = _0x101 + 100;
    v = charstr_parse_signed(_0x101, &end, 0);
    if (v != 257 || errno || end != _0x101 + 6) {
        fprintf(stderr,
                "charstr_parse_signed(_0x101, ... + 5, 16) "
                "=> (%llu, %d, %d)\n",
                (unsigned long long) v, errno, (int) (end - _0x101));
        return false;
    }
    end = _0x101 + 1;
    v = charstr_parse_signed(_0x101, &end, 2);
    if (v || errno != EILSEQ) {
        fprintf(stderr,
                "charstr_parse_signed(_0x101, ... + 5, 2) => (%llu, %d)\n",
                (unsigned long long) v, errno);
        return false;
    }
    return true;
}

static bool test_parse_signed_range(void)
{
    int64_t v;
    errno = 0;
    v = charstr_parse_signed("-0x8000000000000000", NULL, 0);
    if (v != -0x8000000000000000LL || errno) {
        fprintf(stderr,
                "charstr_parse_signed(\"-0x8000000000000000\", NULL, 0) "
                "=> (%llu, %d)\n",
                (unsigned long long) v, errno);
        return false;
    }
    v = charstr_parse_signed("+0x7fffffffffffffff", NULL, 0);
    if (v != 0x7fffffffffffffffLL || errno) {
        fprintf(stderr,
                "charstr_parse_signed(\"+0x7fffffffffffffff\", NULL, 0) "
                "=> (%llu, %d)\n",
                (unsigned long long) v, errno);
        return false;
    }
    v = charstr_parse_signed("0x8000000000000000", NULL, 0);
    if (v != -0x8000000000000000LL || errno != ERANGE) {
        fprintf(stderr,
                "charstr_parse_signed(\"0x8000000000000000\", NULL, 0) "
                "=> (%llu, %d)\n",
                (unsigned long long) v, errno);
        return false;
    }
    return true;
}

static bool test_parse_signed(void)
{
    return test_parse_signed_inval() && 
        test_parse_signed_unlimited() &&
        test_parse_signed_limited() &&
        test_parse_signed_range();
}

int main()
{
    if (!test_decode_utf8_codepoint())
        return EXIT_FAILURE;
    if (!test_ends_with())
        return EXIT_FAILURE;
    if (!test_canonical_nfc())
        return EXIT_FAILURE;
    if (!test_canonical_nfd())
        return EXIT_FAILURE;
    if (!test_canonical_decomposition())
        return EXIT_FAILURE;
    if (!test_canonical_recomposition())
        return EXIT_FAILURE;
    if (!test_sanitization())
        return EXIT_FAILURE;
    if (!test_url_encoding())
        return EXIT_FAILURE;
    if (!test_upper_lower())
        return EXIT_FAILURE;
    if (!test_parse_digits())
        return EXIT_FAILURE;
    if (!test_parse_signed())
        return EXIT_FAILURE;
    fprintf(stderr, "Ok\n");
    return EXIT_SUCCESS;
}
