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

static intset_t *cat_prepend;
static intset_t *cat_cr;
static intset_t *cat_lf;
static intset_t *cat_control;
static intset_t *cat_extend;
static intset_t *cat_ri;
static intset_t *cat_sm;
static intset_t *cat_l;
static intset_t *cat_v;
static intset_t *cat_t;
static intset_t *cat_lv;
static intset_t *cat_lvt;
static intset_t *cat_zwj;

static void make_categories()
{
    cat_prepend = make_intset(N_CP);
    cat_cr = make_intset(N_CP);
    cat_lf = make_intset(N_CP);
    cat_control = make_intset(N_CP);
    cat_extend = make_intset(N_CP);
    cat_ri = make_intset(N_CP);
    cat_sm = make_intset(N_CP);
    cat_l = make_intset(N_CP);
    cat_v = make_intset(N_CP);
    cat_t = make_intset(N_CP);
    cat_lv = make_intset(N_CP);
    cat_lvt = make_intset(N_CP);
    cat_zwj = make_intset(N_CP);
}

static void destroy_categories()
{
    destroy_intset(cat_prepend);
    destroy_intset(cat_cr);
    destroy_intset(cat_lf);
    destroy_intset(cat_control);
    destroy_intset(cat_extend);
    destroy_intset(cat_ri);
    destroy_intset(cat_sm);
    destroy_intset(cat_l);
    destroy_intset(cat_v);
    destroy_intset(cat_t);
    destroy_intset(cat_lv);
    destroy_intset(cat_lvt);
    destroy_intset(cat_zwj);
}

static void add_to_category(int start, int finish, const char *category)
{
    intset_t *set;
    if (!strcmp(category, "Prepend"))
        set = cat_prepend;
    else if (!strcmp(category, "CR"))
        set = cat_cr;
    else if (!strcmp(category, "LF"))
        set = cat_lf;
    else if (!strcmp(category, "Control"))
        set = cat_control;
    else if (!strcmp(category, "Extend"))
        set = cat_extend;
    else if (!strcmp(category, "Regional_Indicator"))
        set = cat_ri;
    else if (!strcmp(category, "SpacingMark"))
        set = cat_sm;
    else if (!strcmp(category, "L"))
        set = cat_l;
    else if (!strcmp(category, "V"))
        set = cat_v;
    else if (!strcmp(category, "T"))
        set = cat_t;
    else if (!strcmp(category, "LV"))
        set = cat_lv;
    else if (!strcmp(category, "LVT"))
        set = cat_lvt;
    else if (!strcmp(category, "ZWJ"))
        set = cat_zwj;
    else
        assert(false);
    for (int cp = start; cp <= finish; cp++)
        intset_add(set, cp);
}

static void generate_category(intset_t *set, const char *cat)
{
    printf("\n"
           "bool charset_unicode_grapheme_break_prop_is_%s(int cp)\n"
           "{\n"
           "    switch (cp) {\n",
           cat);
    /* Condense long, sequential ranges */
    enum { CUT = 20 };
    int largest = -1;
    for (int cp = 0; cp < N_CP; cp++)
        if (intset_has(set, cp)) {
            int begin = cp;
            for (cp++; cp < N_CP && intset_has(set, cp); cp++)
                ;
            int end = cp--;
            largest = cp;
            if (end - begin < CUT)
                for (int n = begin; n < end; n++)
                    printf("        case %d:\n", n);
        }
    printf("            return true;\n"
           "        default:\n"
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
    generate_category(cat_prepend, "prepend");
    generate_category(cat_cr, "cr");
    generate_category(cat_lf, "lf");
    generate_category(cat_control, "control");
    generate_category(cat_extend, "extend");
    generate_category(cat_ri, "ri");
    generate_category(cat_sm, "sm");
    generate_category(cat_l, "l");
    generate_category(cat_v, "v");
    generate_category(cat_t, "t");
    generate_category(cat_lv, "lv");
    generate_category(cat_lvt, "lvt");
    generate_category(cat_zwj, "zwj");
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
