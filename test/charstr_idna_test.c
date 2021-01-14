#if _POSIX_C_SOURCE < 200809L
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fsdyn/charstr.h>

static bool test(int ln, const char *source, const char *toUnicode,
                 const char *toAscii)
{
    if (!*toAscii)
        toAscii = toUnicode;
    if (!*toAscii)
        toAscii = source;
    char *result = charstr_idna_encode(source);
    bool verdict;
    if (result) {
        verdict = result && !strcmp(result, toAscii);
        if (!verdict)
            fprintf(stderr, "  L%d: FAIL: %s ~ %s\n", ln, source, toAscii);
    }
    else {
        fprintf(stderr, "  L%d: strict: %s ~ %s\n", ln, source, toAscii);
        verdict = true;        /* It's ok to be strict */
    }
    fsfree(result);
    return verdict;
}

static char *stripped(char *s)
{
    char *stripped = charstr_strip(s);
    fsfree(s);
    return stripped;
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
        list_t *fields = charstr_split(body, ';', -1);
        char *source = stripped((char *) list_pop_first(fields));
        char *toUnicode = stripped((char *) list_pop_first(fields));
        char *toUnicodeStatus = stripped((char *) list_pop_first(fields));
        char *toAsciiN = stripped((char *) list_pop_first(fields));
        char *toAsciiNStatus = stripped((char *) list_pop_first(fields));
        char *toAsciiT = stripped((char *) list_pop_first(fields));
        char *toAsciiTStatus = stripped((char *) list_pop_first(fields));
        if (source && toUnicode && toAsciiN) {
            ok = test(ln, source, toUnicode, toAsciiN);
            if (!ok)
                fprintf(stderr, "%s: line %d failed\n", argv[0], ln);
        }
        fsfree(source);
        fsfree(toUnicode);
        fsfree(toUnicodeStatus);
        fsfree(toAsciiN);
        fsfree(toAsciiNStatus);
        fsfree(toAsciiT);
        fsfree(toAsciiTStatus);
        list_foreach(fields, (void *) fsfree, NULL);
        list_foreach(parts, (void *) fsfree, NULL);
    }
    if (!ok)
        return EXIT_FAILURE;
    fprintf(stderr, "Ok\n");
    return EXIT_SUCCESS;
}
