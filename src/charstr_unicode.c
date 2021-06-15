#include <errno.h>

#include "charstr.h"
#include "fsdyn_version.h"

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
    return charstr_detect_unicode_normal_form(s, end, UNICODE_NFC_DISALLOWED,
                                              UNICODE_NFC_MAYBE) == 0;
}

bool charstr_unicode_canonically_decomposed(const char *s, const char *end)
{
    return charstr_detect_unicode_normal_form(s, end, UNICODE_NFD_DISALLOWED,
                                              UNICODE_NFD_MAYBE) == 0;
}
