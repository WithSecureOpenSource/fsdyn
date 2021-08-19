#!/usr/bin/env python

# Parse DerivedNormalizationProps.txt

import sys

def main():
    qc_types = [ "NFC_QC", "NFD_QC", "NFKC_QC", "NFKD_QC" ]
    qc = []
    for i in range(0x110000):
        record = {}
        for qc_type in qc_types:
            record[qc_type] = "Y"
        qc.append(record)
    for line in open(sys.argv[1]):
        if "#" in line:
            line = line.split("#", 1)[0]
        line = line.strip()
        if not line:
            continue
        try:
            cp_range, qc_type, qc_value = line.split(";")
        except ValueError:
            continue
        qc_type = qc_type.strip()
        if qc_type not in qc_types:
            continue
        if ".." in cp_range:
            cp_low, cp_high = cp_range.split("..")
            cp_low = int(cp_low, 16)
            cp_high = int(cp_high, 16)
        else:
            cp_low = cp_high = int(cp_range, 16)
        qc_value = qc_value.strip()
        for cp in range(cp_low, cp_high + 1):
            qc[cp][qc_type] = qc_value
    sys.stdout.write(r"""#include "charstr.h"

int charstr_allowed_unicode_normal_forms(int codepoint)
{
    switch (codepoint) {
""")
    for codepoint, record in enumerate(qc):
        disj = []
        for form in [ "NFC", "NFD", "NFKC", "NFKD" ]:
            value = record["%s_QC" % form]
            if value == "N":
                disj.append("UNICODE_%s_DISALLOWED" % form)
            elif value == "M":
                disj.append("UNICODE_%s_MAYBE" % form)
            else:
                assert value == "Y"
        if disj:
            sys.stdout.write("        case %d:\n" % codepoint)
            line = "            return %s;" % " | ".join(disj)
            while len(line) > 80:
                n = line.rindex("|", 0, 80) + 1
                sys.stdout.write("%s\n" % line[:n])
                line = "               %s" % line[n:]
            sys.stdout.write("%s\n" % line)
    sys.stdout.write(r"""        default:
            return 0;
    }
}
""")

if __name__ == '__main__':
    main()
