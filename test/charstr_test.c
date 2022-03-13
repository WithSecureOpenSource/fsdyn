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

static bool charstr_to_unsigned_error(const char *buffer, size_t size,
                                      unsigned base, uint64_t u, ssize_t count)
{
    fprintf(stderr,
            "charstr_to_unsigned(\"%s\", %d, %u, &u) => (%llu, %d, %s)\n",
            buffer, (int) size, base, (unsigned long long) u, (int) count,
            strerror(errno));
    return false;
}

static bool test_to_unsigned_inval(void)
{
    uint64_t u = 777;
    errno = EAGAIN;
    ssize_t count = charstr_to_unsigned("101", -1, 1, &u);
    if (count >= 0 || errno != EINVAL || u != 777)
        return charstr_to_unsigned_error("101", -1, 1, u, count);
    errno = EAGAIN;
    count = charstr_to_unsigned("101", -1, 17, &u);
    if (count >= 0 || errno != EINVAL || u != 777)
        return charstr_to_unsigned_error("101", -1, 17, u, count);
    return true;
}

static bool test_to_unsigned_unlimited(void)
{
    uint64_t u;
    errno = EAGAIN;
    ssize_t count = charstr_to_unsigned("101", -1, 2, &u);
    if (count < 0 || u != 5 || errno != EAGAIN)
        return charstr_to_unsigned_error("101", -1, 2, u, count);
    count = charstr_to_unsigned("101", -1, 5, &u);
    if (count < 0 || u != 26 || errno != EAGAIN)
        return charstr_to_unsigned_error("101", -1, 5, u, count);
    count = charstr_to_unsigned("101", -1, 10, &u);
    if (count < 0 || u != 101 || errno != EAGAIN)
        return charstr_to_unsigned_error("101", -1, 10, u, count);
    count = charstr_to_unsigned("101", -1, 16, &u);
    if (count < 0 || u != 257 || errno != EAGAIN)
        return charstr_to_unsigned_error("101", -1, 16, u, count);
    u = 777;
    count = charstr_to_unsigned("", -1, 10, &u);
    if (count >= 0 || errno != EILSEQ || u != 777)
        return charstr_to_unsigned_error("", -1, 10, u, count);
    errno = EAGAIN;
    count = charstr_to_unsigned("A", -1, 10, &u);
    if (count >= 0 || errno != EILSEQ || u != 777)
        return charstr_to_unsigned_error("A", -1, 10, u, count);
    return true;
}

static bool test_to_unsigned_limited(void)
{
    uint64_t u;
    errno = EAGAIN;
    ssize_t count = charstr_to_unsigned("101 ", 5, 10, &u);
    if (count != 3 || u != 101 || errno != EAGAIN)
        return charstr_to_unsigned_error("101 ", 5, 10, u, count);
    count = charstr_to_unsigned("101 ", 3, 10, &u);
    if (count != 3 || u != 101 || errno != EAGAIN)
        return charstr_to_unsigned_error("101 ", 3, 10, u, count);
    count = charstr_to_unsigned("101 ", 2, 10, &u);
    if (count != 2 || u != 10 || errno != EAGAIN)
        return charstr_to_unsigned_error("101 ", 2, 10, u, count);
    u = 777;
    count = charstr_to_unsigned("101 ", 0, 10, &u);
    if (count >= 0 || errno != EILSEQ)
        return charstr_to_unsigned_error("101 ", 0, 10, u, count);
    errno = EAGAIN;
    count = charstr_to_unsigned("A ", 1, 10, &u);
    if (count >= 0 || errno != EILSEQ || u != 777)
        return charstr_to_unsigned_error("A ", 1, 10, u, count);
    return true;
}

static bool test_to_unsigned_range(void)
{
    uint64_t u;
    errno = EAGAIN;
    ssize_t count = charstr_to_unsigned("0000000000000000", -1, 16, &u);
    if (count < 0 || u || errno != EAGAIN)
        return charstr_to_unsigned_error("0000000000000000", -1, 16, u, count);
    count = charstr_to_unsigned("8000000000000000", -1, 16, &u);
    if (count < 0 || u != 0x8000000000000000 || errno != EAGAIN)
        return charstr_to_unsigned_error("8000000000000000", -1, 16, u, count);
    count = charstr_to_unsigned("ffffffffffffffff", -1, 16, &u);
    if (count < 0 || u != 0xffffffffffffffff || errno != EAGAIN)
        return charstr_to_unsigned_error("ffffffffffffffff", -1, 16, u, count);
    count = charstr_to_unsigned("18446744073709551615", -1, 10, &u);
    if (count < 0 || u != 18446744073709551615ULL || errno != EAGAIN)
        return charstr_to_unsigned_error("18446744073709551615", -1, 10, u,
                                         count);
    count = charstr_to_unsigned("ffffffffffffffffc", -1, 16, &u);
    if (count < 0 || u != 0xfffffffffffffffc || errno != ERANGE)
        return charstr_to_unsigned_error("ffffffffffffffffc", -1, 16, u, count);
    return true;
}

static bool test_to_unsigned(void)
{
    return test_to_unsigned_inval() && test_to_unsigned_unlimited() &&
        test_to_unsigned_limited() && test_to_unsigned_range();
}

static bool charstr_to_integer_error(const char *buffer, size_t size,
                                     unsigned base, int64_t v, ssize_t count)
{
    fprintf(stderr,
            "charstr_to_integer(\"%s\", %d, %u, &u) => (%lld, %d, %s)\n",
            buffer, (int) size, base, (long long) v, (int) count,
            strerror(errno));
    return false;
}

static bool test_to_integer_inval(void)
{
    int64_t v;
    errno = EAGAIN;
    ssize_t count = charstr_to_integer("101", -1, 1, &v);
    if (count >= 0 || errno != EINVAL)
        return charstr_to_integer_error("101", -1, 1, v, count);
    errno = EAGAIN;
    count = charstr_to_integer("101", -1, 17, &v);
    if (count >= 0 || errno != EINVAL)
        return charstr_to_integer_error("101", -1, 17, v, count);
    return true;
}

static bool test_to_integer_unlimited(void)
{
    int64_t v;
    errno = EAGAIN;
    ssize_t count = charstr_to_integer("12", -1, 2, &v);
    if (count < 0 || v != 1 || errno != EAGAIN)
        return charstr_to_integer_error("12", -1, 2, v, count);
    count = charstr_to_integer("-12", -1, 8, &v);
    if (count < 0 || v != -10 || errno != EAGAIN)
        return charstr_to_integer_error("-12", -1, 8, v, count);
    count = charstr_to_integer("-12", -1, 0, &v);
    if (count < 0 || v != -12 || errno != EAGAIN)
        return charstr_to_integer_error("-12", -1, 0, v, count);
    count = charstr_to_integer("+0x12", -1, 0, &v);
    if (count < 0 || v != 18 || errno != EAGAIN)
        return charstr_to_integer_error("+0x12", -1, 0, v, count);
    count = charstr_to_integer("+012", -1, 0, &v);
    if (count < 0 || v != 10 || errno != EAGAIN)
        return charstr_to_integer_error("+012", -1, 0, v, count);
    v = 777;
    count = charstr_to_integer("0xQ", -1, 0, &v);
    if (count >= 0 || errno != EILSEQ || v != 777)
        return charstr_to_integer_error("0xQ", -1, 0, v, count);
    errno = EAGAIN;
    count = charstr_to_integer(" 0x0", -1, 0, &v);
    if (count >= 0 || errno != EILSEQ || v != 777)
        return charstr_to_integer_error(" 0x0", -1, 0, v, count);
    return true;
}

static bool test_to_integer_limited(void)
{
    int64_t v;
    errno = EAGAIN;
    ssize_t count = charstr_to_integer("0x101 ", 4, 2, &v);
    if (count != 1 || v || errno != EAGAIN)
        return charstr_to_integer_error("0x101 ", 4, 2, v, count);
    count = charstr_to_integer("0x101 ", 4, 0, &v);
    if (count != 4 || v != 16 || errno != EAGAIN)
        return charstr_to_integer_error("0x101 ", 4, 0, v, count);
    count = charstr_to_integer("0x101 ", 100, 0, &v);
    if (count != 5 || v != 257 || errno != EAGAIN)
        return charstr_to_integer_error("0x101 ", 100, 0, v, count);
    return true;
}

static bool test_to_integer_range(void)
{
    int64_t v;
    errno = EAGAIN;
    ssize_t count = charstr_to_integer("-0x8000000000000000", -1, 0, &v);
    if (count < 0 || v != -0x8000000000000000LL || errno != EAGAIN)
        return charstr_to_integer_error("-0x8000000000000000", -1, 0, v, count);
    count = charstr_to_integer("+0x7fffffffffffffff", -1, 0, &v);
    if (count < 0 || v != 0x7fffffffffffffffLL || errno != EAGAIN)
        return charstr_to_integer_error("+0x7fffffffffffffff", -1, 0, v, count);
    count = charstr_to_integer("0x8000000000000000", -1, 0, &v);
    if (count < 0 || v != -0x8000000000000000LL || errno != ERANGE)
        return charstr_to_integer_error("0x8000000000000000", -1, 0, v, count);
    return true;
}

static bool test_to_integer(void)
{
    return test_to_integer_inval() && test_to_integer_unlimited() &&
        test_to_integer_limited() && test_to_integer_range();
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
    if (!test_to_unsigned())
        return EXIT_FAILURE;
    if (!test_to_integer())
        return EXIT_FAILURE;
    fprintf(stderr, "Ok\n");
    return EXIT_SUCCESS;
}
