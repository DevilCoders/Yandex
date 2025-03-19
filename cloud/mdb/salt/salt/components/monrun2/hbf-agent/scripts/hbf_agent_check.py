#!/usr/bin/env python
"""
Copy-pasted from hbf-agent-mds-config package
"""

import sys
import json
import time
import urllib2

URI = "http://localhost:9876/status"
TIMEWINDOW = 600
FUTUREWINDOW = -1

try:
    status = json.load(urllib2.urlopen(URI))
    ts_now = int(time.time())
    ts_delta = ts_now - int(status['last_update'])
    if ts_delta < FUTUREWINDOW:
        print("2;crit, ts from agent from future, agent ts: %s, but now ts is: %s" % (str(status['last_update']), str(ts_now)))
        sys.exit(0)
    elif ts_delta > TIMEWINDOW:
        print("2;crit, ts from agent too old, agent ts: %s, but now ts is: %s" % (str(status['last_update']), str(ts_now)))
        sys.exit(0)
    else:
        pass
    if status['status'] == "OK":
        print("0;ok")
        sys.exit(0)
    elif status['status'] == "WARN":
        print("1;warn, error: %s" % str(status['desc']))
        sys.exit(0)
    elif status['status'] == "CRIT":
        print("2;crit, error: %s" % str(status['desc']))
        sys.exit(0)
    else:
        print("2;crit, UNKNOWN STATUS: %s" % str(status['status']))
        sys.exit(0)
except Exception as e:
        print("1;crit, Exception: %s" % str(e).replace("\n", " "))
        sys.exit(0)

