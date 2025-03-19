#!/usr/bin/env python

from __future__ import print_function
import re
import sys

processors = [
        (re.compile(r'^(COUNT.*)\n$'), lambda line, r: r.sub(r'\n\1', line)),
        (re.compile(r'^"lib_(.*)_timer"\n$'), lambda line, r: "{:30s}".format(r.sub(r'\n\1 ', line))),
        (re.compile(r'^\s+"95%": (\d+)(\.\d+)?,\n$'), lambda line, r: "{:5d} ms".format(int(r.sub(r'\1', line))/1000000)),
        (re.compile(r'^\s+"1m\.rate": (\d+)(\.\d+)?,\n$'), lambda line, r: "{:5s}".format(r.sub(r'\1 ', line))),
        ]


filename = sys.argv[1]
for line in open(filename):
    for regexp, repl in processors:
        if regexp.match(line) is not None:
            print(repl(line, regexp), end='')
            break
print()

