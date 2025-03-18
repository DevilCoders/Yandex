#!/usr/bin/env python2.5

import sys

q = ""
urls = set()

for line in sys.stdin:
    if line.startswith("url="):
        urls.add(line[4:])
        continue
    if line.startswith("req="):
        q = line[4:]
        continue
    if line.startswith("PAGE MARK"):
        for url in urls:
            sys.stdout.write(url)
            sys.stdout.write(q)
        q = ""
        urls = set()
        continue

