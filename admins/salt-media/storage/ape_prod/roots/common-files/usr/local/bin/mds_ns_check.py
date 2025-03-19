#!/usr/bin/env python

import sys
import json
import urllib2

# real example: http://storage-int.mdst.yandex.net/statistics-corestore-staging
mds_url = "http://storage-int.mdst.localhost/statistics-your_ns"

graphitefile = "/usr/lib/yandex-graphite-checks/enabled/mds_ns_check.py"
if len(sys.argv) > 2:
    if len(sys.argv[2]) > 1:
        graphitefile = sys.argv[2]
nameforsender = graphitefile.split("/")[-1]
iamsender = False
if sys.argv[0] == nameforsender or sys.argv[0] == graphitefile or sys.argv[2] == "graphite":
    iamsender = True

if len(sys.argv) > 1:
    if len(sys.argv[1]) > 1:
        mds_url = sys.argv[1]
else:
    if iamsender:
        print("error 1")
        sys.exit(0)
    else:
        print("1;warn, MDS url for check not passed")
        sys.exit(0)

try:
    counter_name = "mds_" + sys.argv[1].split("/")[-1].replace("-", "_")
    stat = json.load(urllib2.urlopen(sys.argv[1]))
    if len(stat) == 0:
        if iamsender:
            print("error 2")
            sys.exit(0)
        else:
            print("2;crit, empty state, mastermind dont give us our state")
            sys.exit(0)
    free_p = 100 - (stat['effective_free_space'] * 100 / stat['total_space'])
except Exception, e:
    if iamsender:
        print("error 2")
        sys.exit(0)
    else:
        print("2;crit, message: %s" % str(e))
        sys.exit(0)

warn = 20
crit = 10
try:
    if len(sys.argv) > 3:
        if len(sys.argv[3]) > 2:
            warncrit = sys.argv[3].split(":")
            warn = int(warncrit[0])
            crit = int(warncrit[1])
except:
    if iamsender:
        print("error 3")
        sys.exit(0)
    else:
        print("2;crit, cant parse: %s. \"WARN:CRIT\" must be [0-9]{2}:[0-9]{2}" % sys.argv[3])
        sys.exit(0)

if free_p < crit:
    if iamsender:
        print("%s %s" % (counter_name, free_p))
        sys.exit(0)
    else:
        print("2;crit, free space at %s less then %i%%: %s" % (counter_name, crit, free_p))
        sys.exit(0)
elif free_p < warn:
    if iamsender:
        print("%s %s" % (counter_name, free_p))
        sys.exit(0)
    else:
        print("1;warn, free space at %s less then %i%%: %s" % (counter_name, warn, free_p))
        sys.exit(0)
else:
    if iamsender:
        print("%s %s" % (counter_name, free_p))
        sys.exit(0)
    else:
        print("0;ok, free space at %s: %s%%" % (counter_name, free_p))
        sys.exit(0)

