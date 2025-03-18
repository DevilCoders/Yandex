#!/usr/bin/env python

# read tweak_log from stdin

import sys
from collections import defaultdict

TWEAKS = defaultdict(lambda:0)

for i in sys.stdin:
    twStr = i.split('\t')[4]
    twks = map(lambda x: x.split()[0], twStr.split(';'))
    for x in twks:
        TWEAKS[x] += 1
        

keys = sorted(TWEAKS.keys(), cmp=lambda l,r: TWEAKS[l] - TWEAKS[r], reverse=True)
for k in keys:
    print '%s   %d' % (k, TWEAKS[k])
