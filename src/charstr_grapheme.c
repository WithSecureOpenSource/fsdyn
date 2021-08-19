#include "charstr.h"

/* https://unicode.org/reports/tr29/#Grapheme_Cluster_Boundaries */

static const char *skip_trailer(const char *s1, const char *s2, int cp1,
                                const char *end)
{
    while (charset_unicode_grapheme_break_prop_is_extend(cp1) ||
           charset_unicode_grapheme_break_prop_is_zwj(cp1) ||
           charset_unicode_grapheme_break_prop_is_sm(cp1)) {
        if (s2 == end)
            return s2;
        s1 = s2;
        s2 = charstr_decode_utf8_codepoint(s1, end, &cp1);
        if (!s2)
            return NULL;
    }
    return s1;
}

static const char *skip_emoji_modifier_sequence(const char *s1, const char *s2,
                                                int cp1, const char *end)
{
    for (;;) {
        while (charset_unicode_grapheme_break_prop_is_extend(cp1)) {
            if (s2 == end)
                return s2;
            s1 = s2;
            s2 = charstr_decode_utf8_codepoint(s1, end, &cp1);
            if (!s2)
                return NULL;
        }
        if (!charset_unicode_grapheme_break_prop_is_zwj(cp1))
            return skip_trailer(s1, s2, cp1, end);
        if (s2 == end)
            return s2;
        s1 = s2;
        s2 = charstr_decode_utf8_codepoint(s1, end, &cp1);
        if (!s2)
            return NULL;
        if (!charset_unicode_emoji_prop_is_extended_pictographic(cp1))
            return skip_trailer(s1, s2, cp1, end);
        if (s2 == end)
            return s2;
        s1 = s2;
        s2 = charstr_decode_utf8_codepoint(s1, end, &cp1);
        if (!s2)
            return NULL;
    }
}

static const char *skip_emoji_flag_sequence(const char *s1, const char *s2,
                                            int cp1, const char *end)
{
    if (!charset_unicode_grapheme_break_prop_is_ri(cp1))
        return skip_trailer(s1, s2, cp1, end);
    if (s2 == end)
        return s2;
    s1 = charstr_decode_utf8_codepoint(s2, end, &cp1);
    if (!s1)
        return NULL;
    return skip_trailer(s2, s1, cp1, end);
}

const char *charstr_skip_utf8_grapheme(const char *s, const char *end)
{
    int cp0, cp1;
    const char *s1 = charstr_decode_utf8_codepoint(s, end, &cp0);
    if (!s1)
        return NULL;
    if (s1 == end)
        return s1;
    const char *s2 = charstr_decode_utf8_codepoint(s1, end, &cp1);
    if (!s2)
        return NULL;
    if (charset_unicode_emoji_prop_is_extended_pictographic(cp0))
        return skip_emoji_modifier_sequence(s1, s2, cp1, end);
    if (charset_unicode_grapheme_break_prop_is_ri(cp0))
        return skip_emoji_flag_sequence(s1, s2, cp1, end);
    for (;;) {
        if (cp0 == '\r' && cp1 == '\n')
            return s2;
        if (cp0 == '\r' || cp0 == '\n' ||
            charset_unicode_grapheme_break_prop_is_control(cp0))
            return s1;
        if (cp1 == '\r' || cp1 == '\n' ||
            charset_unicode_grapheme_break_prop_is_control(cp1))
            return s1;
        if ((!charset_unicode_grapheme_break_prop_is_l(cp0) ||
             (!charset_unicode_grapheme_break_prop_is_l(cp1) &&
              !charset_unicode_grapheme_break_prop_is_v(cp1) &&
              !charset_unicode_grapheme_break_prop_is_lv(cp1) &&
              !charset_unicode_grapheme_break_prop_is_lvt(cp1))) &&
            ((!charset_unicode_grapheme_break_prop_is_lv(cp0) &&
              !charset_unicode_grapheme_break_prop_is_v(cp0)) ||
             (!charset_unicode_grapheme_break_prop_is_v(cp1) &&
              !charset_unicode_grapheme_break_prop_is_t(cp1))) &&
            ((!charset_unicode_grapheme_break_prop_is_lvt(cp0) &&
              !charset_unicode_grapheme_break_prop_is_t(cp0)) ||
             !charset_unicode_grapheme_break_prop_is_t(cp1)) &&
            (!charset_unicode_grapheme_break_prop_is_extend(cp1) &&
             !charset_unicode_grapheme_break_prop_is_zwj(cp1)) &&
            !charset_unicode_grapheme_break_prop_is_sm(cp1) &&
            !charset_unicode_grapheme_break_prop_is_prepend(cp0))
            break;
        cp0 = cp1;
        s1 = s2;
        if (s1 == end)
            return s1;
        s2 = charstr_decode_utf8_codepoint(s1, end, &cp1);
        if (!s2)
            return NULL;
    }
    return s1;
}
