#!/usr/bin/env python
#-*- coding: utf-8 -*-

import re
import sys

def process(inp, out):
    while True:
        line = inp.readline()
        if not line:
            break
        line = line.rstrip("\n")

        if line.startswith("@"):
            continue

        line = line.replace("/", "")
        line = line.replace(",praet,aor", ",aor")
        line = line.replace(",oo", "")
        line = line.replace(",proper,geo", ",geo")
        line = re.sub("\s+Approximate", "", line)
        line = line.replace(",proper,persn", ",persn")
        line = line.replace("qR,", "NUM,")
        line = line.replace("NUMPRO", "ADV")

        if ",pf" in line and ",ipf" in line:
            line = line.replace(",ipf", "")
            print >>out, line
            line = line.replace(",pf", ",ipf")

        print >>out, line

if __name__ == "__main__":
    inp = sys.stdin
    out = sys.stdout

    process(inp, out)

    inp.close()
    out.close()

