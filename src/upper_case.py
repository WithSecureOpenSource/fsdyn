#!/usr/bin/env python

import sys

def main():
    sys.stdout.write(r"""#include "charstr.h"

int charstr_naive_ucase_unicode(int codepoint)
{
    switch (codepoint) {
""")
    prev_cp = -1
    range_start = None
    for line in open(sys.argv[1]):
        codepoint_str, descr, category, _ = line.split(";", 3)
        codepoint = int(codepoint_str, 16)
        if descr.endswith(", Last>"):
            assert range_start == category
            filler = category
            range_start = None
        else:
            assert range_start is None
            filler = "Cn"
            if descr.endswith(", First>"):
                range_start = category
        if category == "Ll":
            try:
                sys.stdout.write(
                    "        case %d: return %d;\n" % (
                        codepoint, int(line.split(";")[12], 16)))
            except ValueError:
                pass
        prev_cp = codepoint
    assert range_start is None
    sys.stdout.write("""        default: return codepoint;
    }
}\n
""")

if __name__ == '__main__':
    main()
