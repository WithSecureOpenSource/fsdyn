#if _POSIX_C_SOURCE < 200809L
#define _GNU_SOURCE
#endif
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fsdyn/charstr.h>
#include <fsdyn/integer.h>
#include <fsdyn/list.h>

static char *add_codepoint(const char *begin, const char *end, char *outp,
                           const char *outendp)
{
    if (!begin)
        return outp;
    int cp = 0;
    while (begin != end) {
        unsigned digit = charstr_digit_value(*begin++);
        assert(digit != -1);
        cp = cp * 16 + digit;
    }
    return charstr_encode_utf8_codepoint(cp, outp, outendp);
}

static bool fail()
{
    return false;
}

static bool test(const char *testcase)
{
    const char *end = testcase + strlen(testcase);
    const char *tp = testcase;
    const char *begin_value = NULL;
    char outbuf[500];
    char *outp = outbuf;
    char *outendp = outbuf + sizeof outbuf - 1;
    list_t *offsets = make_list();
    while (*tp) {
        int cp;
        const char *nextp = charstr_decode_utf8_codepoint(tp, end, &cp);
        assert(nextp);
        switch (cp) {
            case '\t':
            case ' ':
            case 0xd7: /* × */
                outp = add_codepoint(begin_value, tp, outp, outendp);
                begin_value = NULL;
                break;
            case 0xf7: /* ÷ */
                outp = add_codepoint(begin_value, tp, outp, outendp);
                begin_value = NULL;
                if (outp != outbuf)
                    list_append(offsets, as_integer(outp - outbuf));
                break;
            default:
                assert(cp < 256);
                if (!begin_value)
                    begin_value = tp;
        }
        tp = nextp;
    }
    outp = add_codepoint(begin_value, tp, outp, outendp);
    assert(outp);
    *outp = '\0';
    bool ok = true;
    if (outp != outbuf) {
        const char *p = outbuf;
        while (p != outp) {
            p = charstr_skip_utf8_grapheme(p, outp);
            if (!p || list_empty(offsets) ||
                as_intptr(list_pop_first(offsets)) != p - outbuf) {
                ok = fail();
                break;
            }
        }
        if (!list_empty(offsets))
            ok = fail();
    }
    destroy_list(offsets);
    return ok;
}

int main(int argc, const char *const *argv)
{
    FILE *f = fopen(argv[1], "r");
    char *line = NULL;
    size_t length = 0;
    bool ok = true;
    for (int ln = 1; ok && getline(&line, &length, f) > 0; ln++) {
        list_t *parts = charstr_split(line, '#', 1);
        const char *body = list_elem_get_value(list_get_first(parts));
        ok = test(body);
        list_foreach(parts, (void *) fsfree, NULL);
        destroy_list(parts);
    }
    free(line);
    fclose(f);
    if (!ok)
        return EXIT_FAILURE;
    fprintf(stderr, "Ok\n");
    return EXIT_SUCCESS;
}
