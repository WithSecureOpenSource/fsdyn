#!/usr/bin/env python

import sys

def main():
    recompositions = {}
    exclusions = set()
    for line in open(sys.argv[2]):
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        exclusions.add(int(line.split()[0], 16))
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
        starter, cc = decomposition
        if codepoint not in exclusions:
            if cc not in recompositions:
                recompositions[cc] = {}
            recompositions[cc][starter] = codepoint
    sys.stdout.write("""#include "charstr.h"
""")
    emit_recompositions(recompositions)

def emit_recompositions(recompositions):
    for cc in sorted(recompositions):
        sys.stdout.write("""
static int recompose_{}(int starter)
{{
    switch (starter) {{
""".format(cc))
        for starter, codepoint in sorted(recompositions[cc].items()):
            sys.stdout.write("""        case {}:
            return {};
""".format(starter, codepoint))
        sys.stdout.write("""        default:
            return -1;
    }
}
""")
    sys.stdout.write("""
int _charstr_unicode_primary_composite(int starter, int cc)
{
    switch (cc) {
""")
    for cc in sorted(recompositions):
        sys.stdout.write("""        case {}:
            return recompose_{}(starter);
""".format(cc, cc))
    sys.stdout.write("""        default:
            return -1;
    }
}
""")

if __name__ == '__main__':
    main()
