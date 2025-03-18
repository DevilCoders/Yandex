#!/usr/local/bin/python

# set all reqs from 95.188.180.151 as robot (after first)

import sys;

wasRobot = False;

for line in sys.stdin:
    if not line.startswith("95.188.180.151"):
        print line,;
        continue;

    if line.endswith("1\n"):
        wasRobot = True;
        print line,;
        continue;

    if not wasRobot:
        print line,;
        continue;

    print "%s\t1" % line.split("\t")[0];

