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

static intset_t *cat_emoji;
static intset_t *cat_emoji_presentation;
static intset_t *cat_emoji_modifier;
static intset_t *cat_emoji_modifier_base;
static intset_t *cat_emoji_component;
static intset_t *cat_extended_pictographic;

static void make_categories()
{
    cat_emoji = make_intset(N_CP);
    cat_emoji_presentation = make_intset(N_CP);
    cat_emoji_modifier = make_intset(N_CP);
    cat_emoji_modifier_base = make_intset(N_CP);
    cat_emoji_component = make_intset(N_CP);
    cat_extended_pictographic = make_intset(N_CP);
}

static void destroy_categories()
{
    destroy_intset(cat_emoji);
    destroy_intset(cat_emoji_presentation);
    destroy_intset(cat_emoji_modifier);
    destroy_intset(cat_emoji_modifier_base);
    destroy_intset(cat_emoji_component);
    destroy_intset(cat_extended_pictographic);
}

static void add_to_category(int start, int finish, const char *category)
{
    intset_t *set;
    if (!strcmp(category, "Emoji"))
        set = cat_emoji;
    else if (!strcmp(category, "Emoji_Presentation"))
        set = cat_emoji_presentation;
    else if (!strcmp(category, "Emoji_Modifier"))
        set = cat_emoji_modifier;
    else if (!strcmp(category, "Emoji_Modifier_Base"))
        set = cat_emoji_modifier_base;
    else if (!strcmp(category, "Emoji_Component"))
        set = cat_emoji_component;
    else if (!strcmp(category, "Extended_Pictographic"))
        set = cat_extended_pictographic;
    else
        assert(false);
    for (int cp = start; cp <= finish; cp++)
        intset_add(set, cp);
}

static void generate_category(intset_t *set, const char *cat)
{
    printf("\n"
           "bool charset_unicode_emoji_prop_is_%s(int cp)\n"
           "{\n"
           "    switch (cp) {\n",
           cat);
    /* Condense long, sequential ranges */
    enum { CUT = 20 };
    int largest = -1;
    bool case_found = false;
    for (int cp = 0; cp < N_CP; cp++)
        if (intset_has(set, cp)) {
            int begin = cp;
            for (cp++; cp < N_CP && intset_has(set, cp); cp++)
                ;
            int end = cp--;
            largest = cp;
            if (end - begin < CUT)
                for (int n = begin; n < end; n++) {
                    case_found = true;
                    printf("        case %d:\n", n);
                }
        }
    if (case_found)
        printf("            return true;\n");
    printf("        default:\n"
           "            if (cp > %d)\n"
           "                return false;\n",
           largest);
    for (int cp = 0; cp < N_CP; cp++)
        if (intset_has(set, cp)) {
            int begin = cp;
            for (cp++; cp < N_CP && intset_has(set, cp); cp++)
                ;
            int end = cp--;
            if (end - begin >= CUT)
                printf("            if (cp < %d)\n"
                       "                return false;\n"
                       "            if (cp < %d)\n"
                       "                return true;\n",
                       begin, end);
        }
    printf("            return false;\n"
           "    }\n"
           "}\n");
}

static void generate_categories()
{
    printf("#include <stdbool.h>\n");
    generate_category(cat_emoji, "emoji");
    generate_category(cat_emoji_presentation, "emoji_presentation");
    generate_category(cat_emoji_modifier, "emoji_modifier");
    generate_category(cat_emoji_modifier_base, "emoji_modifier_base");
    generate_category(cat_emoji_component, "emoji_component");
    generate_category(cat_extended_pictographic, "extended_pictographic");
}

int main(int argc, const char *const *argv)
{
    make_categories();
    FILE *f = fopen(argv[1], "r");
    char *line = NULL;
    size_t length = 0;
    while (getline(&line, &length, f) > 0) {
        list_t *parts = charstr_split(line, '#', 1);
        const char *body = list_elem_get_value(list_get_first(parts));
        list_t *fields = charstr_split(body, ';', -1);
        char *cp_range = (char *) list_pop_first(fields);
        assert(cp_range);
        char *value = (char *) list_pop_first(fields);
        if (value) {
            list_t *range = charstr_split_str(cp_range, "..", -1);
            char *first = (char *) list_pop_first(range);
            char *last = (char *) list_pop_first(range);
            if (!last)
                last = charstr_dupstr(first);
            int start = strtol(first, NULL, 16);
            int finish = strtol(last, NULL, 16);
            char *category = charstr_strip(value);
            add_to_category(start, finish, category);
            fsfree(category);
            fsfree(first);
            fsfree(last);
            list_foreach(range, (void *) fsfree, NULL);
            destroy_list(range);
            fsfree(value);
        }
        fsfree(cp_range);
        list_foreach(fields, (void *) fsfree, NULL);
        destroy_list(fields);
        list_foreach(parts, (void *) fsfree, NULL);
        destroy_list(parts);
    }
    free(line);
    generate_categories();
    destroy_categories();
    return EXIT_SUCCESS;
}
