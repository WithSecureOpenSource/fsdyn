#include <string.h>
#include <errno.h>
#include "charstr.h"
#include "avltree_version.h"

charstr_unicode_category_t charstr_unicode_category(int codepoint)
{
    extern const uint8_t _charstr_unicode_categories[0x110000];
    return _charstr_unicode_categories[codepoint];
}

int charstr_naive_lcase_unicode(int codepoint)
{
    extern const uint8_t _charstr_unicode_lower_case[0x110000];
    return _charstr_unicode_lower_case[codepoint];
}

int charstr_naive_ucase_unicode(int codepoint)
{
    extern const uint8_t _charstr_unicode_upper_case[0x110000];
    return _charstr_unicode_upper_case[codepoint];
}

int charstr_allowed_unicode_normal_forms(int codepoint)
{
    extern const uint8_t _charstr_allowed_unicode_normal_forms[0x110000];
    return _charstr_allowed_unicode_normal_forms[codepoint];
}

int charstr_unicode_canonical_combining_class(int codepoint)
{
    extern const uint8_t _charstr_unicode_canonical_combining_class[0x110000];
    return _charstr_unicode_canonical_combining_class[codepoint];
}

int charstr_detect_unicode_normal_form(const char *s, const char *end,
                                       int disallowed_flag, int maybe_flag)
{
    int last_canonical_class = 0;
    int result = 0;
    while (s != end && *s) {
        int codepoint;
        s = charstr_decode_utf8_codepoint(s, end, &codepoint);
        if (!s) {
            errno = EILSEQ;
            return -1;
        }
        int ccc = charstr_unicode_canonical_combining_class(codepoint);
        if (ccc && last_canonical_class > ccc)
            return disallowed_flag;
        int check = charstr_allowed_unicode_normal_forms(codepoint);
        if (check & disallowed_flag)
            return disallowed_flag;
        if (check & maybe_flag)
            result = maybe_flag;
        last_canonical_class = ccc;
    }
    return result;
}

bool charstr_unicode_canonically_composed(const char *s, const char *end)
{
    return charstr_detect_unicode_normal_form(s, end,
                                              UNICODE_NFC_DISALLOWED,
                                              UNICODE_NFC_MAYBE) == 0;
}

bool charstr_unicode_canonically_decomposed(const char *s, const char *end)
{
    return charstr_detect_unicode_normal_form(s, end,
                                              UNICODE_NFD_DISALLOWED,
                                              UNICODE_NFD_MAYBE) == 0;
}

static char *decompose_fully(int codepoint, char *s, const char *end)
{
    extern const int *_charstr_unicode_decompositions[0x110000];
    const int *dp = _charstr_unicode_decompositions[codepoint];
    if (!dp)
        return charstr_encode_utf8_codepoint(codepoint, s, end);
    while (s && *dp)
        s = decompose_fully(*dp++, s, end);
    return s;
}

static void sort_combining_characters(char *begin, const char *end)
{
    char *s = begin;
    int last_ccc = 0;
    while (s != end) {
        int codepoint;
        const char *after_s = charstr_decode_utf8_codepoint(s, end, &codepoint);
        int ccc = charstr_unicode_canonical_combining_class(codepoint);
        if (ccc == 0) {
            begin = (char *) after_s;
            last_ccc = 0;
        } else if (ccc >= last_ccc)
            last_ccc = ccc;
        else {
            /* Not very efficient... */
            char *t = begin;
            for (;;) {
                int t_cp;
                const char *after_t =
                    charstr_decode_utf8_codepoint(t, end, &t_cp);
                int t_ccc = charstr_unicode_canonical_combining_class(t_cp);
                if (t_ccc > ccc) {
                    size_t s_count = after_s - s;
                    char snippet[s_count];
                    memcpy(snippet, s, s_count);
                    memmove(t + s_count, t, s - t);
                    memcpy(t, snippet, s_count);
                    break;
                }
                t = (char *) after_t;
            }
        }
        s = (char *) after_s;
    }
}

char *charstr_unicode_decompose(const char *s, const char *end,
                                char output[], const char *output_end)
{
    char *q = output;
    while (s != end && *s) {
        int codepoint;
        s = charstr_decode_utf8_codepoint(s, end, &codepoint);
        if (!s) {
            errno = EILSEQ;
            return NULL;
        }
        q = decompose_fully(codepoint, q, output_end);
        if (!q) {
            errno = EOVERFLOW;
            return NULL;
        }
    }
    if (q == output_end) {
        errno = EOVERFLOW;
        return NULL;
    }
    *q = '\0';
    sort_combining_characters(output, q);
    return q;
}

char *charstr_unicode_recompose(const char *s, const char *end,
                                char output[], const char *output_end)
{
    extern const char *_charstr_unicode_compose(const char *s,
                                                const char *end,
                                                int *codepoint);
    char *q = output;
    while (s != end && *s) {
        int codepoint;
        s = _charstr_unicode_compose(s, end, &codepoint);
        if (!s) {
            errno = EILSEQ;
            return NULL;
        }
        q = charstr_encode_utf8_codepoint(codepoint, q, output_end);
        if (!q) {
            errno = EOVERFLOW;
            return NULL;
        }
    }
    if (q == output_end) {
        errno = EOVERFLOW;
        return NULL;
    }
    *q = '\0';
    return q;
}
