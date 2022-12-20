#if _POSIX_C_SOURCE < 200809L
#define _GNU_SOURCE
#endif
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "charstr.h"
#include "intset.h"

enum {
    N_CP = 0x110000,
};

static const char *disallowed_property_names[] = {
    "UNICODE_NFC_DISALLOWED",
    "UNICODE_NFD_DISALLOWED",
    "UNICODE_NFKC_DISALLOWED",
    "UNICODE_NFKD_DISALLOWED",
};

static const char *maybe_property_names[] = {
    "UNICODE_NFC_MAYBE",
    "UNICODE_NFD_MAYBE",
    "UNICODE_NFKC_MAYBE",
    "UNICODE_NFKD_MAYBE",
};

static int parse_property_name(const char *str)
{
    if (!strcmp(str, "NFC_QC"))
        return 0;
    if (!strcmp(str, "NFD_QC"))
        return 1;
    if (!strcmp(str, "NFKC_QC"))
        return 2;
    if (!strcmp(str, "NFKD_QC"))
        return 3;
    return -1;
}

static void read_normalization_props(const char *path,
                                     intset_t *unicode_disallowed[4],
                                     intset_t *unicode_maybe[4])
{
    FILE *f = fopen(path, "r");
    char *line = NULL;
    size_t length = 0;

    for (int i = 0; i < 4; i++) {
        unicode_disallowed[i] = make_intset(N_CP);
        unicode_maybe[i] = make_intset(N_CP);
    }

    while (getline(&line, &length, f) > 0) {
        char *fields[3];
        char *ptr = strchr(line, '#');
        if (ptr)
            *ptr = '\0';
        unsigned n = charstr_split_into_array(line, ';', fields, 2);
        if (n == 2) {
            for (int i = 0; i <= n; i++) {
                char *str = charstr_strip(fields[i]);
                fsfree(fields[i]);
                fields[i] = str;
            }

            int property_id = parse_property_name(fields[1]);
            if (property_id != -1) {
                char *end;
                int first = strtol(fields[0], &end, 16);
                int last = first;
                if (end[0] == '.' && end[1] == '.')
                    last = strtol(&end[2], NULL, 16);

                for (int cp = first; cp <= last; cp++)
                    switch (*fields[2]) {
                        case 'N':
                            intset_add(unicode_disallowed[property_id], cp);
                            break;
                        case 'M':
                            intset_add(unicode_maybe[property_id], cp);
                            break;
                        default:
                            assert(false);
                    }
            }
        }

        for (int i = 0; i <= n; i++)
            fsfree(fields[i]);
    }
    fclose(f);
    free(line);
}

int main(int argc, const char *const *argv)
{
    intset_t *unicode_disallowed[4];
    intset_t *unicode_maybe[4];
    read_normalization_props(argv[1], unicode_disallowed, unicode_maybe);

    printf("#include \"charstr.h\"\n"
           "\n"
           "int charstr_allowed_unicode_normal_forms(int codepoint)\n"
           "{\n"
           "    switch (codepoint) {\n");

    for (int cp = 0; cp < N_CP; cp++) {
        list_t *props = make_list();
        for (int i = 0; i < 4; i++) {
            if (intset_has(unicode_disallowed[i], cp))
                list_append(props, disallowed_property_names[i]);
            else if (intset_has(unicode_maybe[i], cp))
                list_append(props, maybe_property_names[i]);
        }

        if (list_size(props)) {
            char *value = charstr_join(" | ", props);
            printf("        case %d:\n"
                   "            return %s;\n",
                   cp, value);
            fsfree(value);
        }

        destroy_list(props);
    }

    printf("        default:\n"
           "            return 0;\n"
           "    }\n"
           "}\n");

    for (int i = 0; i < 4; i++) {
        destroy_intset(unicode_disallowed[i]);
        destroy_intset(unicode_maybe[i]);
    }

    return EXIT_SUCCESS;
}
