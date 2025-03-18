#!/usr/bin/env python

import fileinput
import sys
import re

def main(args):
    duration = re.compile("^0\.[0-9]*s$")

    input = fileinput.input(args[1:])

    for line in input:
        line = line.rstrip("\r\n")
        if line == "Wizard initialized":
            break

    for line in input:
        line = line.rstrip("\r\n")
        parts = line.split("\t")
        if len(parts) == 2 and duration.match(parts[1]):
            print line[:-1]
    return 0

main(sys.argv)
