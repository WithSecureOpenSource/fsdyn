#!/usr/bin/env python

import sys

def main():
    decompositions = {}
    for line in open(sys.argv[1]):
        fields = line.split(";")
        hex_cp = fields[0]
        codepoint = int(hex_cp, 16)
        raw_decomposition = fields[5]
        if not raw_decomposition:
            continue
        if raw_decomposition.startswith("<"):
            continue            # non-canonical decomposition; ignore
        decomposition = [ int(dc, 16) for dc in raw_decomposition.split() ]
        if len(decomposition) < 2:
            continue
        cursor = decompositions
        for dc in decomposition:
            if dc not in cursor:
                cursor[dc] = {}
            cursor = cursor[dc]
        cursor[None] = codepoint
    sys.stdout.write("""#include "charstr.h"

""")
    compose_root(decompositions)

def compose_root(decompositions):
    prefix = ["recompose"]
    items = sorted(decompositions.items())
    for key, value in items:
        if key is not None:
            prefix.append("%x" % key)
            compose(value, prefix)
            prefix.pop()
    sys.stdout.write("""const char *
_charstr_unicode_compose(const char *s, const char *end, int *codepoint)
{
    const char *t = charstr_decode_utf8_codepoint(s, end, codepoint);
    if (!t)
        return NULL;
    for (;;) {
        int candidate = *codepoint;
        const char *u;
        switch (*codepoint) {
""")
    for key, value in items:
        sys.stdout.write("""            case 0x%x:
                u = %s_%x(t, end, codepoint);
                break;
""" % (key, "_".join(map(str, prefix)), key))
    sys.stdout.write("""            default:
                u = NULL;
        }
        if (!u) {
            *codepoint = candidate;
            return t;
        }
        t = u; 
    }
}
""")

def compose(decompositions, prefix):
    items = sorted(decompositions.items())
    for key, value in items:
        if key is not None:
            prefix.append("%x" % key)
            compose(value, prefix)
            prefix.pop()
    sys.stdout.write("""static const char *
%s(const char *s, const char *end, int *codepoint)
{
""" % "_".join(map(str, prefix)))
    if None in decompositions:
        sys.stdout.write("""    *codepoint = 0x%x;
    return s;
}

""" % decompositions[None])
        return
    sys.stdout.write("""    const char *t = charstr_decode_utf8_codepoint(s, end, codepoint);
    if (!t)
        return NULL;
    switch (*codepoint) {
""")
    for key, value in items:
        sys.stdout.write("""        case 0x%x:
            return %s_%x(t, end, codepoint);
""" % (key, "_".join(map(str, prefix)), key))
    sys.stdout.write("""        default:
            return NULL;
    }
}

""")

if __name__ == '__main__':
    main()
