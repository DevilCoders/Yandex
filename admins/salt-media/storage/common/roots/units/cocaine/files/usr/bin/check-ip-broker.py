#!/usr/bin/python

import os
import sys
import json
import requests

ISOLATE_CONFIG_PATH = '/etc/cocaine-isolate-daemon/cocaine-isolate-daemon.conf'
QUOTA = 10000000

try:
    with open(ISOLATE_CONFIG_PATH) as f:
        isolate_config = json.load(f)
except Exception as e:
    print("2; CRIT, cant read isolate config: %s" % str(e))
    sys.exit(0)

try:
    url = isolate_config['mtn']['url'] + "/count"
    headers = isolate_config['mtn']['headers']
except Exception as e:
    print("2; CRIT, cant get params from isolate config: %s" % str(e))
    sys.exit(0)

timeout = 9
count = -1
quota = -1

try:
    r = requests.get(url, headers=headers, timeout=timeout)
    answer = r.json()
    count = answer['count']
    quota = answer['quota']
except Exception as e:
    print("2; CRIT, cant get information from ip-broker: %s" % str(e))
    sys.exit(0)

if QUOTA > quota:
    print("2; CRIT, ip-broker report that out quota less, %s,then %s" % (quota, QUOTA))
    sys.exit(0)

quota_usage = int(100 * float(count)/float(quota))
if quota_usage > 70:
    print("2; CRIT, we use 70%%+ our quota: %d" % quota_usage)
    sys.exit(0)

if quota_usage > 50:
    print("1; WARN, we use 50%%+ of our quota: %d" % quota_usage)
    sys.exit(0)

print("0; OK, we use %d%% (%d) of our quota: %d" % (quota_usage, count, quota))
sys.exit(0)

