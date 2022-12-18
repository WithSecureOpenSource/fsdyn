#if _POSIX_C_SOURCE < 200809L
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "charstr.h"
#include "integer.h"

int main(int argc, const char *const *argv)
{
    FILE *f = fopen(argv[1], "r");
    char *line = NULL;
    size_t length = 0;
    list_t *decompositions = make_list();

    printf("#include <stddef.h>\n");

    while (getline(&line, &length, f) > 0) {
        char *fields[15];
        charstr_split_into_array(line, ';', fields, 14);
        int cp = strtol(fields[0], NULL, 16);

        if (*fields[5] && *fields[5] != '<') {
            list_t *decomposition = charstr_split(fields[5], ' ', -1);

            printf("static const int decompose_%x[] = {", cp);
            for (list_elem_t *e = list_get_first(decomposition); e;
                 e = list_next(e)) {
                int n = strtol(list_elem_get_value(e), NULL, 16);
                printf(" 0x%x,", n);
            }
            printf(" 0 };\n");
            list_append(decompositions, as_integer(cp));

            list_foreach(decomposition, (void *) fsfree, NULL);
            destroy_list(decomposition);
        }

        for (int i = 0; i < 15; i++)
            fsfree(fields[i]);
    }
    free(line);

    printf("\n"
           "const int *_charstr_unicode_decomposition(int codepoint)\n"
           "{\n"
           "    switch (codepoint) {\n");

    for (list_elem_t *e = list_get_first(decompositions); e; e = list_next(e)) {
        int cp = as_intptr(list_elem_get_value(e));
        printf("        case %d:\n"
               "            return decompose_%x;\n",
               cp, cp);
    }
    destroy_list(decompositions);

    printf("        default:\n"
           "            return NULL;\n"
           "    }\n"
           "}\n");

    return EXIT_SUCCESS;
}
