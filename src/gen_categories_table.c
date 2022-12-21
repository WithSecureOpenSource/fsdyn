#if _POSIX_C_SOURCE < 200809L
#define _GNU_SOURCE
#endif
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "charstr.h"
#include "list.h"

enum {
    N_CP = 0x110000,
};

typedef struct {
    int first;
    int last;
    char *category;
} range_t;

static list_t *build_category_ranges(char **categories, char **names)
{
    list_t *ranges = make_list();
    range_t *range = NULL;
    for (int cp = 0; cp < N_CP; cp++)
        if (categories[cp]) {
            if (charstr_ends_with(names[cp], ", Last>")) {
                assert(range &&
                       charstr_ends_with(names[range->first], ", First>") &&
                       !strcmp(range->category, categories[cp]));
                range->last = cp;
            } else if (range && !strcmp(range->category, categories[cp]) &&
                       cp == range->last + 1) {
                range->last = cp;
            } else {
                range = fsalloc(sizeof(range_t));
                range->first = cp;
                range->last = cp;
                range->category = categories[cp];
                list_append(ranges, range);
            }
        }
    return ranges;
}

static void emit_table(range_t **ranges, size_t size, int level)
{
    char *indent = charstr_printf("%*c", level * 4, ' ');
    if (size < 5) {
        for (int i = 0; i < size; i++) {
            if (ranges[i]->first == ranges[i]->last)
                printf("%sif (codepoint < %d)\n"
                       "%s    return UNICODE_CATEGORY_%s;\n",
                       indent, ranges[i]->last + 1, indent,
                       ranges[i]->category);
            else
                printf("%sif (codepoint < %d) {\n"
                       "%s    if (codepoint >= %d)\n"
                       "%s        return UNICODE_CATEGORY_%s;\n"
                       "%s    return UNICODE_CATEGORY_Cn;\n"
                       "%s}\n",
                       indent, ranges[i]->last + 1, indent, ranges[i]->first,
                       indent, ranges[i]->category, indent, indent);
        }
    } else {
        size_t middle = size / 5;
        printf("%sif (codepoint < %d) {\n", indent, ranges[middle]->last + 1);
        emit_table(ranges, middle, level + 1);
        if (ranges[middle]->first == ranges[middle]->last)
            printf("%s    return UNICODE_CATEGORY_%s;\n"
                   "%s}\n",
                   indent, ranges[middle]->category, indent);
        else
            printf("%s    if (codepoint >= %d)\n"
                   "%s        return UNICODE_CATEGORY_%s;\n"
                   "%s    return UNICODE_CATEGORY_Cn;\n"
                   "%s}\n",
                   indent, ranges[middle]->first, indent,
                   ranges[middle]->category, indent, indent);
        emit_table(&ranges[middle + 1], size - middle - 1, level);
    }
    fsfree(indent);
}

int main(int argc, const char *const *argv)
{
    FILE *f = fopen(argv[1], "r");
    char *line = NULL;
    size_t length = 0;
    char **categories = fscalloc(N_CP, sizeof(char *));
    char **names = fscalloc(N_CP, sizeof(char *));

    while (getline(&line, &length, f) > 0) {
        char *fields[15];
        charstr_split_into_array(line, ';', fields, 14);
        int cp = strtol(fields[0], NULL, 16);
        names[cp] = fields[1];
        fields[1] = NULL;
        categories[cp] = fields[2];
        fields[2] = NULL;

        for (int i = 0; i < 15; i++)
            fsfree(fields[i]);
    }
    fclose(f);
    free(line);

    list_t *ranges = build_category_ranges(categories, names);
    size_t size = list_size(ranges);
    range_t **ranges_arr = fscalloc(size, sizeof(range_t));
    for (int i = 0; i < size; i++)
        ranges_arr[i] = (range_t *) list_pop_first(ranges);
    destroy_list(ranges);

    printf(
        "#include \"charstr.h\"\n"
        "\n"
        "charstr_unicode_category_t charstr_unicode_category(int codepoint)\n"
        "{\n");

    emit_table(ranges_arr, size, 1);

    printf("\n"
           "    return UNICODE_CATEGORY_Cn;\n"
           "}\n");

    for (int i = 0; i < size; i++)
        fsfree(ranges_arr[i]);
    fsfree(ranges_arr);

    for (int cp = 0; cp < N_CP; cp++) {
        fsfree(categories[cp]);
        fsfree(names[cp]);
    }
    fsfree(categories);
    fsfree(names);

    return EXIT_SUCCESS;
}
