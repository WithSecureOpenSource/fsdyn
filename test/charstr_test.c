#include <stdio.h>
#include <string.h>
#include <stdlib.h>
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
    const char *bad_utf8 = "¿Qué ha pasado? "
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

static bool test_punycode_encoding(void)
{
    struct {
        const char *input, *output;
    } data[] = {
        { "你好你好", "xn--6qqa088eba" },
        { "hyvää.yötä", "xn--hyv-slaa.xn--yt-wia4e" },
        { "hyvää.yötä.", "xn--hyv-slaa.xn--yt-wia4e." },
        { "ä.ö", "xn--4ca.xn--nda" },
        { "Ä.Ö.", "xn--4ca.xn--nda." },
        { NULL }
    };
    for (int i; data[i].input; i++) {
        char *encoding = charstr_punycode_encode(data[i].input);
        if (!encoding)
            return false;
        if (strcmp(encoding, data[i].output)) {
            fsfree(encoding);
            return false;
        }
        fsfree(encoding);
    }
    return true;
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
    if (!test_punycode_encoding())
        return EXIT_FAILURE;
    fprintf(stderr, "Ok\n");
    return EXIT_SUCCESS;
}
