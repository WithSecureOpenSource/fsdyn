#!/usr/bin/env python

import sys

def main():
    sys.stdout.write("""#include "charstr.h"

const uint8_t _charstr_unicode_categories[0x110000] = {
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
        for _ in range(prev_cp + 1, codepoint):
            sys.stdout.write("    UNICODE_CATEGORY_%s,\n" % filler)
        sys.stdout.write("    UNICODE_CATEGORY_%s,\n" % category)
        prev_cp = codepoint
    assert range_start is None
    sys.stdout.write("""};\n
""")

if __name__ == '__main__':
    main()
