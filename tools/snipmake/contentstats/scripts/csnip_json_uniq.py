#!/usr/bin/env python
import json
import sys

qurls = set()

for line in sys.stdin:
    line = line.strip()
    if not line:
        continue
    js = json.loads(line)
    key = (js[0]['userreq'], js[0]['url'])
    if key not in qurls:
        qurls.add(key)
        print line

print >>sys.stderr, len(qurls)
