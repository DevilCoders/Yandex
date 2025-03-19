#!/usr/bin/env python
import sys

for line in sys.stdin:
    text = line.strip()
    print text.decode("utf8").lower().encode("utf8")
