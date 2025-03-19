#!/usr/bin/env python
import json
import requests
import sys
import datetime
import time

crit_time = 500


def metrics():
    requests.packages.urllib3.disable_warnings()
    url = "https://localhost:1443/stats/buckets"
    while True:
        try:
            r = requests.get(url, timeout=10, verify=False)
            stats = json.loads(r.text)
            for m in stats["results"]:
                yield m
            if stats["next"] is None:
                break
            url = "https://localhost:1443/stats/buckets?cursor={}".format(stats["next"])
        except Exception:
            print "1; {} not available".format(url)
            exit(0)


def die(status, message):
    print "%d;%s" % (status, message)
    sys.exit(2)


old_stats = {}
num_stats = 0
for i in metrics():
    num_stats += 1
    try:
        dt = datetime.datetime.strptime(i['updated_ts'], '%Y-%m-%dT%H:%M:%S')
    except Exception:
        continue
    stat_time = time.mktime(dt.timetuple())
    nt = datetime.datetime.utcnow()
    now_time = time.mktime(nt.timetuple())
    delta = nt - dt
    if delta.seconds > crit_time:
        old_stats[i['name']] = delta.seconds


def first(n, data):
    return dict(data.items()[:n])


if len(old_stats) > 0:
    first_3 = first(3, old_stats)
    die(2, "{} from {} records staled: {}...".format(len(old_stats),
                                                     num_stats,
                                                     first_3))
else:
    die(0, 'Ok {} records'.format(num_stats))
