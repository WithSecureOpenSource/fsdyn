#include <string.h>
#include <unistd.h>
#include <assert.h>
#include "charstr.h"

enum {
    MAX_DNS_NAME_LENGTH = 254,
    MAX_DNS_LABEL_LENGTH = 63,
    BASE = 36,
    TMIN = 1,
    TMAX = 26,
    SKEW = 38,
    DAMP = 700,
    INITIAL_BIAS = 72,
    INITIAL_N = 128
};

static int adapt(int delta, size_t numpoints, bool firsttime)
{
    if (firsttime)
        delta /= DAMP;
    else delta /= 2;
    delta += delta / numpoints;
    int k;
    for (k = 0; delta > (BASE - TMIN) * TMAX / 2; k += BASE)
        delta /= BASE - TMIN;
    return k + (BASE - TMIN + 1) * delta / (delta + SKEW);
}

static void record_nonascii(int codepoint, int nonascii[],
                            size_t *nonascii_count)
{
    size_t c = *nonascii_count;
    size_t i;
    for (i = 0; i < c && nonascii[i] <= codepoint; i++)
        if (nonascii[i] == codepoint)
            return;
    memmove(nonascii + i + 1, nonascii + i, (c - i) * sizeof *nonascii);
    nonascii[i] = codepoint;
    *nonascii_count = c + 1;
}

static char *fail(const char *reason)
{
    /* An auxiliary function to help in troubleshooting. Set your
     * breakpoint here. */
    return NULL;
}

static const char *punycode_encode_pass1(const char *p, const char *end,
                                         size_t *pc_count,
                                         char ascii[], size_t *ascii_count,
                                         int nonascii[],
                                         size_t *nonascii_count)
{
    if (p == end)
        return fail("label is empty");
    if (p[0] == '-')
        return fail("label begins with hyphen");
    if (end[-1] == '-')
        return fail("label ends with hyphen");
    bool first = true;
    size_t n, h = 0;
    *nonascii_count = 0;
    for (n = 0; p != end && *p != '.'; n++) {
        int codepoint;
        p = charstr_decode_utf8_codepoint(p, end, &codepoint);
        if (!p)
            return fail("bad UTF-8 in pass1");
        if (codepoint < 128) {
            if (codepoint != '-' &&
                !(charstr_char_class(codepoint) & CHARSTR_ALNUM))
                return fail("disallowed ASCII");
            ascii[h++] = charstr_lcase_char(codepoint);
        } else if (first &&
                   charstr_unicode_category(codepoint) == UNICODE_CATEGORY_Mc)
            return fail("label begins with composing mark");
        else record_nonascii(codepoint, nonascii, nonascii_count);
        first = false;
    }
    *pc_count = n;
    *ascii_count = h;
    return p;
}

static const char BASE36_DIGIT[] = "abcdefghijklmnopqrstuvwxyz0123456789";

static char *emit(char *o, char *end_output, size_t *output_size, char c)
{
    (*output_size)++;
    if (!o || o >= end_output)
        return NULL;
    *o++ = c;
    return o;
}

static char *punycode_encode_pass2(const char *input, const char *end,
                                   char *o, char *end_output,
                                   size_t *output_size, size_t ascii_count,
                                   int nonascii[], size_t nonascii_count)
{
    int n = INITIAL_N;
    int delta = 0;
    int bias = INITIAL_BIAS;
    size_t handled = ascii_count;
    size_t basic_count = handled;
    for (size_t ni = 0; ni < nonascii_count; ni++) {
        delta += (nonascii[ni] - n) * (handled + 1);
        n = nonascii[ni];
        const char *p = input;
        while (p != end && *p != '.') {
            int codepoint;
            p = charstr_decode_utf8_codepoint(p, end, &codepoint);
            if (codepoint != n) {
                if (codepoint < n)
                    delta++;
                continue;
            }
            int q = delta;
            for (int k = BASE;; k += BASE) {
                int t;
                if (k <= bias)
                    t = TMIN;
                else if (k >= bias + TMAX)
                    t = TMAX;
                else t = k - bias;
                if (q < t)
                    break;
                o = emit(o, end_output, output_size,
                         BASE36_DIGIT[t + (q - t) % (BASE - t)]);
                q = (q - t) / (BASE - t);
            }
            o = emit(o, end_output, output_size, BASE36_DIGIT[q]);
            bias = adapt(delta, handled + 1, handled == basic_count);
            delta = 0;
            handled++;
        }
        delta++;
        n++;
    }
    return o;
}

/* Read UTF-8 pointcodes from input until end is hit or '.' is encountered. */
static const char *punycode_encode(const char *input, const char *end,
                                   char *output, size_t *output_size)
{
    char ascii[end - input]; /* nonunique characters in order of appearance */
    size_t ascii_count;
    int nonascii[end - input];  /* unique codepoints in ascending order */
    size_t nonascii_count;
    size_t pc_count;
    const char *next =
        punycode_encode_pass1(input, end, &pc_count, ascii, &ascii_count,
                              nonascii, &nonascii_count);
    if (!next)
        return NULL;
    char *o = output;
    char *end_output = o + *output_size;
    *output_size = 0;
    if (nonascii_count) {
        o = emit(o, end_output, output_size, 'x');
        o = emit(o, end_output, output_size, 'n');
        o = emit(o, end_output, output_size, '-');
        o = emit(o, end_output, output_size, '-');
    }
    for (size_t i = 0; i < ascii_count; i++)
        o = emit(o, end_output, output_size, ascii[i]);
    if (!nonascii_count)
        return next;
    if (ascii_count)
        o = emit(o, end_output, output_size, '-');
    o = punycode_encode_pass2(input, end, o, end_output, output_size,
                              ascii_count, nonascii, nonascii_count);
    if (*output_size > MAX_DNS_LABEL_LENGTH)
        return fail("label too long");
    return next;
}

static bool is_all_ascii(const char *s)
{
    while (*s)
        if (*s++ & 0x80)
            return false;
    return true;
}

static char *encode_filtered(const char *hostname, const char *end)
{
    if (end - hostname > MAX_DNS_NAME_LENGTH) /* encoding won't shorten it */
        return fail("hostname too long");
    const char *p = hostname;
    size_t output_size = 0;
    char encoding[MAX_DNS_NAME_LENGTH + 1];
    char *q = encoding;
    do {
        size_t output_label_size = encoding + sizeof encoding - q;
        p = punycode_encode(p, end, q, &output_label_size);
        if (!p)
            return NULL;
        assert(output_label_size <= MAX_DNS_LABEL_LENGTH);
        output_size += output_label_size + 1;
        if (output_size > MAX_DNS_NAME_LENGTH - 1) /* -1 for the final '.' */
            return fail("hostname encoding too long");
        q += output_label_size;
        *q++ = '.';
    } while (end - p++ > 1);    /* allow the DNS name to end in a '.' */
    output_size += end - p + 1; /* allow a final '.' */
    q[end - p] = '\0';
    return charstr_dupstr(encoding);
}

static char *safecopy(const char *src, char *dest, char *end)
{
    size_t capacity = end - dest;
    size_t size = strlen(src);
    if (!dest || size > capacity)
        return NULL;;
    memcpy(dest, src, size);
    return dest + size;
}

static char *encode_nfc(const char *hostname, const char *end)
{
    const char *p = hostname;
    char workarea[MAX_DNS_NAME_LENGTH + 1];
    char *q = workarea;
    char *workend = workarea + MAX_DNS_NAME_LENGTH;
    while (p != end) {
        int codepoint;
        p = charstr_decode_utf8_codepoint(p, end, &codepoint);
        if (!p)
            return fail("bad UTF-8 in NFC");
        if (charstr_idna_status_is_valid(codepoint) ||
            charstr_idna_status_is_deviation(codepoint))
            q = charstr_encode_utf8_codepoint(codepoint, q, workend);
        else if (charstr_idna_status_is_ignored(codepoint))
            ;
        else if (charstr_idna_status_is_mapped(codepoint))
            q = safecopy(charstr_idna_mapping(codepoint), q, workend);
        else
            /* UseSTD3ASCIIRules == true */
            return fail("disallowed codepoint");
        if (!q)
            return fail("buffer overflow");
    }
    /* TODO: https://unicode.org/reports/tr46/#Processing
     * CheckBidi & CheckJoiners */
    return encode_filtered(workarea, q);
}

static char *encode_nfd(const char *hostname, const char *end)
{
    size_t worklen = end - hostname; /* recomposing won't grow it */
    char *workarea = fsalloc(worklen + 1);
    char *workend = workarea + worklen;
    workend = charstr_unicode_recompose(hostname, end, workarea, workend + 1);
    if (!workend) {
        fsfree(workarea);
        return fail("failed to recompose");
    }
    char *encoding = encode_nfc(workarea, workend);
    fsfree(workarea);
    return encoding;
}

char *charstr_idna_encode(const char *hostname)
{
    if (is_all_ascii(hostname))
        return charstr_lcase_str(charstr_dupstr(hostname));
    size_t length = strlen(hostname);
    const char *end = hostname + length;
    if (charstr_unicode_canonically_composed(hostname, end))
        return encode_nfc(hostname, end);
    if (charstr_unicode_canonically_decomposed(hostname, end))
        return encode_nfd(hostname, end);
    size_t worklen = 3 * length; /* should be enough for decomposing */
    char *workarea = fsalloc(worklen + 1);
    char *workend = workarea + worklen;
    workend = charstr_unicode_decompose(hostname, end, workarea, workend);
    if (!workend) {
        fsfree(workarea);
        return fail("failed to decompose");
    }
    char *encoding = encode_nfd(workarea, workend);
    fsfree(workarea);
    return encoding;
}

const char *charstr_idna_status(int codepoint)
{
    if (charstr_idna_status_is_valid(codepoint))
        return "valid";
    if (charstr_idna_status_is_mapped(codepoint))
        return "mapped";
    if (charstr_idna_status_is_ignored(codepoint))
        return "ignored";
    if (charstr_idna_status_is_deviation(codepoint))
        return "deviation";
    if (charstr_idna_status_is_disallowed(codepoint))
        return "disallowed";
    if (charstr_idna_status_is_disallowed_STD3_valid(codepoint))
        return "disallowed_STD3_valid";
    if (charstr_idna_status_is_disallowed_STD3_mapped(codepoint))
        return "disallowed_STD3_mapped";
    return "?";
}

