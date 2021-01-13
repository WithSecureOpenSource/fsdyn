#include "charstr.h"

list_t *charstr_split_re(const char *s, const regex_t *delim, int eflags,
                         unsigned max_split)
{
    list_t *list = make_list();
    while (max_split--) {
        regmatch_t match;
        if (regexec(delim, s, 1, &match, eflags) != 0)
            break;
        list_append(list, charstr_dupsubstr(s, s + match.rm_so));
        s += match.rm_eo;
    }
    list_append(list, charstr_dupstr(s));
    return list;
}
