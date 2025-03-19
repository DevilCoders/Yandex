#!/usr/bin/env python

from collections import defaultdict

import sys

COMMON_PROP = '\033[92m'
UNCOMMON_PROP = '\033[93m'
RARE_PROP = '\033[91m'
ENDC = '\033[0m'

prop2host = defaultdict(set)
host_count = 0
last_prop_count = 0

cur_host = None

for line in sys.stdin.readlines():
    if len(line.rstrip()) == 0:
        cur_host = None
    elif cur_host is None:
        cur_host = line.split(":")[0]
        host_count += 1
        last_prop_count = 0
    elif not line.startswith("OUT["):
        prop2host[line.rstrip()].add(cur_host)
        last_prop_count += 1

if last_prop_count == 0:
    host_count -= 1

props = sorted(prop2host.items(), key=lambda x: x[0])
print "host count: %s" % host_count
for x, y in props:
    if len(y) > 5:
        print "%s\t%sfound at %s hosts%s" % (x, COMMON_PROP if len(y) == host_count else UNCOMMON_PROP, len(y), ENDC)
    else:
        print "%s\t%sfound only at the following hosts: %s%s" % (x, RARE_PROP, ", ".join(y), ENDC)
