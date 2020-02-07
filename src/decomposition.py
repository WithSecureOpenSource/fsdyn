#!/usr/bin/env python

import sys

def main():
    sys.stdout.write("""#include <stddef.h>

""")
    decompositions = set()
    for line in open(sys.argv[1]):
        fields = line.split(";")
        hex_cp = fields[0]
        codepoint = int(hex_cp, 16)
        raw_decomposition = fields[5]
        if not raw_decomposition:
            continue
        if raw_decomposition.startswith("<"):
            continue            # non-canonical decomposition; ignore
        decomposition = raw_decomposition.split()
        sys.stdout.write(
            "static const int decompose_%x[] = {" % codepoint)
        for cp in decomposition:
            sys.stdout.write(" 0x%x," % int(cp, 16))
        sys.stdout.write(" 0 };\n")
        decompositions.add(codepoint)
    sys.stdout.write("""
const int *_charstr_unicode_decompositions[0x110000] = {
""")
    for codepoint in range(0, 0x110000):
        if codepoint in decompositions:
            sys.stdout.write(
                "    decompose_%x,\n" % codepoint)
        else:
            sys.stdout.write("    NULL,\n")
    sys.stdout.write("};\n")

if __name__ == '__main__':
    main()
