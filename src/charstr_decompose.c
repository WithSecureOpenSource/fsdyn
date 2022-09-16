#include <errno.h>
#include <string.h>

#include "charstr.h"
#include "fsdyn_version.h"

static char *decompose_hangul(int codepoint, char *s, const char *end)
{
    enum {
        S_BASE = 0xAC00,
        L_BASE = 0x1100,
        V_BASE = 0x1161,
        T_BASE = 0x11A7,
        L_COUNT = 19,
        V_COUNT = 21,
        T_COUNT = 28,
        N_COUNT = V_COUNT * T_COUNT,
        S_COUNT = L_COUNT * N_COUNT
    };
    int i = codepoint - S_BASE;
    if (i < 0 || i >= S_COUNT)
        return NULL;
    s = charstr_encode_utf8_codepoint(L_BASE + i / N_COUNT, s, end);
    s = charstr_encode_utf8_codepoint(V_BASE + i % N_COUNT / T_COUNT, s, end);
    if (i % T_COUNT)
        s = charstr_encode_utf8_codepoint(T_BASE + i % T_COUNT, s, end);
    return s;
}

static char *decompose_fully(int codepoint, char *s, const char *end)
{
    extern const int *_charstr_unicode_decomposition(int codepoint);
    const int *dp = _charstr_unicode_decomposition(codepoint);
    if (!dp) {
        char *hangul_end = decompose_hangul(codepoint, s, end);
        if (hangul_end)
            return hangul_end;
        return charstr_encode_utf8_codepoint(codepoint, s, end);
    }
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

char *charstr_unicode_decompose(const char *s, const char *end, char output[],
                                const char *output_end)
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

char *charstr_unicode_convert_to_nfd(const char *s)
{
    if (charstr_unicode_canonically_decomposed(s, NULL)) {
        errno = 0;
        return NULL;
    }
    char *nfd = fsalloc(3 * strlen(s) + 1);
    if (!charstr_unicode_decompose(s, NULL, nfd, NULL)) {
        int err = errno;
        fsfree(nfd);
        errno = err;
        return NULL;
    }
    errno = 0;
    return nfd;
}

char *charstr_unicode_convert_to_nfc(const char *s)
{
    if (charstr_unicode_canonically_composed(s, NULL)) {
        errno = 0;
        return NULL;
    }
    char *nfd = charstr_unicode_convert_to_nfd(s);
    if (errno)
        return NULL;
    if (nfd)
        s = nfd;
    char *nfc = fsalloc(strlen(s) + 1);
    if (!charstr_unicode_recompose(s, NULL, nfc, NULL)) {
        int err = errno;
        fsfree(nfd);
        fsfree(nfc);
        errno = err;
        return NULL;
    }
    fsfree(nfd);
    errno = 0;
    return nfc;
}
