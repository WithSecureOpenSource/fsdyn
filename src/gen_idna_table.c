#if _POSIX_C_SOURCE < 200809L
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "charstr.h"

enum {
    N_CP = 0x110000
};

static struct {
    char *status, *mapping, *idna2008;
} table[N_CP];


static void generate_test(const char *status)
{
    printf("\n"
           "bool charstr_idna_status_is_%s(int codepoint)\n"
           "{\n",
           status);
    int prev_cp = -1;
    for (int cp = 0; cp < N_CP; cp++) {
        if (strcmp(table[cp].status, status)) {
            if (prev_cp >= 0) {
                printf("    if (codepoint < %d) return codepoint >= %d;\n",
                       cp, prev_cp);
                prev_cp = -1;
            }
        } else if (prev_cp < 0)
            prev_cp = cp;
    }
    printf("    return false;\n"
           "}\n");
}

static void generate_mappings()
{
    printf("\n"
           "const char *charstr_idna_mapping(int codepoint)\n"
           "{\n"
           "    switch (codepoint) {\n");
    for (int cp = 0; cp < N_CP; cp++)
        if (table[cp].mapping && *table[cp].mapping) {
            printf("        case %d: return \"", cp);
            for (const char *p = table[cp].mapping; *p; p++)
                printf("\\%o", *p & 0xff);
            printf("\";\n");
        }
    printf("        default: return NULL;\n"
           "    }\n"
           "}\n");
}

int main(int argc, const char *const *argv)
{
    FILE *f = fopen(argv[1], "r");
    char *line = NULL;
    size_t length = 0;
    while (getline(&line, &length, f) > 0) {
        list_t *parts = charstr_split(line, '#', 1);
        const char *body = list_elem_get_value(list_get_first(parts));
        list_t *fields = charstr_split(body, ';', -1);
        char *cp_range = (char *) list_pop_first(fields);
        assert(cp_range);
        char *status = (char *) list_pop_first(fields);
        if (status) {
            char *mapping = (char *) list_pop_first(fields);
            char *idna2008 = NULL;
            if (mapping)
                idna2008 = (char *) list_pop_first(fields); /* may be NULL */
            list_t *range = charstr_split_str(cp_range, "..", -1);
            char *first = (char *) list_pop_first(range);
            char *last = (char *) list_pop_first(range);
            if (!last)
                last = charstr_dupstr(first);
            char buffer[100], *q = buffer, *end = buffer + sizeof buffer - 1;
            if (mapping) {
                list_t *codepoints = charstr_split_atoms(mapping);
                list_elem_t *e;
                for (e = list_get_first(codepoints); e; e = list_next(e)) {
                    int codepoint = strtol(list_elem_get_value(e), NULL, 16);
                    q = charstr_encode_utf8_codepoint(codepoint, q, end);
                    assert(q);
                }
                *q++ = '\0';
                list_foreach(codepoints, (void *) fsfree, NULL);
                destroy_list(codepoints);
            }
            int start = strtol(first, NULL, 16);
            int finish = strtol(last, NULL, 16);
            for (int cp = start; cp <= finish; cp++) {
                assert(!table[cp].status);
                table[cp].status = charstr_strip(status);
                if (mapping)
                    table[cp].mapping = charstr_dupstr(buffer);
                if (idna2008)
                    table[cp].idna2008 = charstr_strip(idna2008);
            }
            fsfree(first);
            fsfree(last);
            list_foreach(range, (void *) fsfree, NULL);
            destroy_list(range);
            fsfree(idna2008);
            fsfree(mapping);
            fsfree(status);
        }
        fsfree(cp_range);
        list_foreach(fields, (void *) fsfree, NULL);
        destroy_list(fields);
        list_foreach(parts, (void *) fsfree, NULL);
        destroy_list(parts);
    }
    free(line);
    for (int cp = 0; cp < N_CP; cp++)
        assert(table[cp].status);
    printf("#include <stddef.h>\n"
           "#include <stdbool.h>\n");
    generate_test("deviation");
    generate_test("disallowed");
    generate_test("disallowed_STD3_valid");
    generate_test("disallowed_STD3_mapped");
    generate_test("ignored");
    generate_test("mapped");
    generate_test("valid");
    generate_mappings();
    return EXIT_SUCCESS;
}
