#!/usr/bin/env python
from __future__ import print_function
import sys


def remove_hilight(s):
    return s.replace(u"\007]", "").replace(u"\007[", "")

for line in sys.stdin:
    text = line.strip().decode("utf8")
    print(remove_hilight(text).encode("utf8"))
