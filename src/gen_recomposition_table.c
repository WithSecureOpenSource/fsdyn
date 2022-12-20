#if _POSIX_C_SOURCE < 200809L
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "avltree.h"
#include "charstr.h"
#include "integer.h"
#include "intset.h"

enum {
    N_CP = 0x110000,
};

static intset_t *read_exclusions(const char *path)
{
    FILE *f = fopen(path, "r");
    char *line = NULL;
    size_t length = 0;
    intset_t *exclusions = make_intset(N_CP);

    while (getline(&line, &length, f) > 0) {
        if (*line && *line != '#') {
            int cp = strtol(line, NULL, 16);
            intset_add(exclusions, cp);
        }
    }
    fclose(f);
    free(line);

    return exclusions;
}

static avl_tree_t **read_recompositions(const char *path, intset_t *exclusions)
{
    FILE *f = fopen(path, "r");
    char *line = NULL;
    size_t length = 0;
    avl_tree_t **recompositions = fscalloc(N_CP, sizeof(avl_tree_t *));

    while (getline(&line, &length, f) > 0) {
        char *fields[15];
        charstr_split_into_array(line, ';', fields, 14);
        int cp = strtol(fields[0], NULL, 16);

        if (!intset_has(exclusions, cp) && *fields[5] && *fields[5] != '<') {
            list_t *decomposition = charstr_split(fields[5], ' ', -1);

            if (list_size(decomposition) == 2) {
                int first = strtol(list_elem_get_value(
                                       list_get_by_index(decomposition, 0)),
                                   NULL, 16);
                int second = strtol(list_elem_get_value(
                                        list_get_by_index(decomposition, 1)),
                                    NULL, 16);

                if (!recompositions[second])
                    recompositions[second] = make_avl_tree(integer_cmp);
                avl_tree_put(recompositions[second], as_integer(first),
                             as_integer(cp));
            }

            list_foreach(decomposition, (void *) fsfree, NULL);
            destroy_list(decomposition);
        }

        for (int i = 0; i < 15; i++)
            fsfree(fields[i]);
    }
    fclose(f);
    free(line);

    return recompositions;
}

int main(int argc, const char *const *argv)
{
    intset_t *exclusions = read_exclusions(argv[2]);
    avl_tree_t **recompositions = read_recompositions(argv[1], exclusions);
    destroy_intset(exclusions);

    printf("#include \"charstr.h\"\n"
           "\n");

    for (int cp = 0; cp < N_CP; cp++)
        if (recompositions[cp]) {
            printf("static int recompose_%d(int starter)\n"
                   "{\n"
                   "    switch (starter) {\n",
                   cp);

            for (avl_elem_t *e = avl_tree_get_first(recompositions[cp]); e;
                 e = avl_tree_next(e)) {
                int first = as_intptr(avl_elem_get_key(e));
                int cp = as_intptr(avl_elem_get_value(e));
                printf("        case %d:\n"
                       "            return %d;\n",
                       first, cp);
            }

            printf("        default:\n"
                   "            return -1;\n"
                   "    }\n"
                   "}\n"
                   "\n");
        }

    printf("int _charstr_unicode_primary_composite(int starter, int cc)\n"
           "{\n"
           "    switch (cc) {\n");

    for (int cp = 0; cp < N_CP; cp++)
        if (recompositions[cp]) {
            printf("        case %d:\n"
                   "            return recompose_%d(starter);\n",
                   cp, cp);
            destroy_avl_tree(recompositions[cp]);
        }
    fsfree(recompositions);

    printf("        default:\n"
           "            return -1;\n"
           "    }\n"
           "}\n");

    return EXIT_SUCCESS;
}
