#!/usr/bin/env python

import sys

try:
    chr = unichr                # python2
except NameError:
    def ord(x): return x        # python3

def main():
    table = [None] * 0x110000
    for line in open(sys.argv[1]):
        try:
            body, comment = line.split("#", 1)
        except ValueError:
            body = line
        fields = body.split(";")
        if len(fields) < 2:
            continue
        if len(fields) == 2:
            cp_range, status = fields
            mapping = idna2008 = None
        if len(fields) == 3:
            cp_range, status, mapping = fields
            idna2008 = None
        if len(fields) == 4:
            cp_range, status, mapping, idna2008 = fields
        try:
            first, last = cp_range.split("..")
        except ValueError:
            first = last = cp_range
        if mapping is not None:
            mapping = "".join([ chr(int(h, 16)) for h in mapping.split() ])
        for cp in range(int(first, 16), int(last, 16) + 1):
            assert table[cp] is None
            table[cp] = (status.strip(), mapping, idna2008)
    assert None not in table
    sys.stdout.write("""#include <stddef.h>
#include <stdbool.h>
""")
    generate_test(table, 'deviation')
    generate_test(table, 'disallowed')
    generate_test(table, 'disallowed_STD3_valid')
    generate_test(table, 'disallowed_STD3_mapped')
    generate_test(table, 'ignored')
    generate_test(table, 'mapped')
    generate_test(table, 'valid')
    generate_mappings(table)

def generate_test(table, target_status):
    sys.stdout.write("""
bool charstr_idna_status_is_{}(int codepoint)
{{
""".format(target_status))
    prev_cp = None
    for cp, (status, mapping, idna2008) in enumerate(table):
        if status != target_status:
            if prev_cp is not None:
                sys.stdout.write(
                    """    if (codepoint < {}) return codepoint >= {};
""".format(cp, prev_cp))
                prev_cp = None
        elif prev_cp is None:
            prev_cp = cp
    sys.stdout.write("""    return false;
}
""")

def generate_mappings(table):
    sys.stdout.write("""
const char *charstr_idna_mapping(int codepoint)
{
    switch (codepoint) {
""")
    for cp, (status, mapping, idna2008) in enumerate(table):
        if status.endswith("mapped"):
            sys.stdout.write('''        case {}: return "'''.format(cp))
            for b in mapping.encode("UTF-8"):
                sys.stdout.write("\\%o" % ord(b))
            sys.stdout.write('''";
''')
    sys.stdout.write("""        default: return NULL;
    }
}
""")

if __name__ == '__main__':
    main()
