#!/usr/bin/env python

import sys

N = 0x110000

def main():
    prev_cp = -1
    range_start = None
    table = [None] * N
    for line in open(sys.argv[1]):
        codepoint_str, descr, category, _ = line.split(";", 3)
        codepoint = int(codepoint_str, 16)
        if descr.endswith(", Last>"):
            assert range_start == category
            range_start = None
            for n in range(prev_cp + 1, codepoint):
                assert table[n] is None
                table[n] = category
        else:
            assert range_start is None
            if descr.endswith(", First>"):
                range_start = category
        table[codepoint] = category
        prev_cp = codepoint
    assert range_start is None
    ranges = []
    n = 0
    while n < N:
        while n < N and table[n] is None:
            n += 1
        if n >= N:
            break
        start = n
        category = table[n]
        while n < N and table[n] == category:
            n += 1
        ranges.append((start, n, category))
    sys.stdout.write(r"""#include "charstr.h"

charstr_unicode_category_t charstr_unicode_category(int codepoint)
{
""")
    emit_binary(ranges, 1)
    sys.stdout.write(r"""
    return UNICODE_CATEGORY_Cn;
}
""")

def emit_binary(ranges, indentation):
    assert ranges
    length = len(ranges)
    I = indentation * "    "
    if length < 5:
        for start, end, category in ranges:
            if end == start + 1:
                sys.stdout.write(
                    r"""%sif (codepoint < %d)
%s    return UNICODE_CATEGORY_%s;
""" % (I, end, I, category))
            else:
                sys.stdout.write(
                    r"""%sif (codepoint < %d) {
%s    if (codepoint >= %d)
%s        return UNICODE_CATEGORY_%s;
%s    return UNICODE_CATEGORY_Cn;
%s}
""" % (I, end, I, start, I, category, I, I))
        return
    middle = length // 5
    start, end, category = ranges[middle]
    sys.stdout.write(
        r"""%sif (codepoint < %d) {
""" % (I, end))
    emit_binary(ranges[0:middle], indentation + 1)
    if end == start + 1:
        sys.stdout.write(
            r"""%s    return UNICODE_CATEGORY_%s;
%s}
""" % (I, category, I))
    else:
        sys.stdout.write(
            r"""%s    if (codepoint >= %d)
%s        return UNICODE_CATEGORY_%s;
%s    return UNICODE_CATEGORY_Cn;
%s}
""" % (I, start, I, category, I, I))
    emit_binary(ranges[middle + 1:], indentation)

if __name__ == '__main__':
    main()
