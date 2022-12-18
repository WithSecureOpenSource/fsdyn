#if _POSIX_C_SOURCE < 200809L
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "charstr.h"

int main(int argc, const char *const *argv)
{
    FILE *f = fopen(argv[1], "r");
    char *line = NULL;
    size_t length = 0;

    printf("#include \"charstr.h\"\n"
           "\n"
           "int charstr_unicode_canonical_combining_class(int codepoint)\n"
           "{\n"
           "    switch (codepoint) {\n");

    while (getline(&line, &length, f) > 0) {
        char *fields[15];
        charstr_split_into_array(line, ';', fields, 14);
        int cp = strtol(fields[0], NULL, 16);

        if (*fields[3]) {
            int canonical_combining_class = strtol(fields[3], NULL, 10);
            printf("        case %d:\n"
                   "            return %d;\n",
                   cp, canonical_combining_class);
        }

        for (int i = 0; i < 15; i++)
            fsfree(fields[i]);
    }
    free(line);

    printf("        default:\n"
           "            return 0;\n"
           "    }\n"
           "}\n");

    return EXIT_SUCCESS;
}
