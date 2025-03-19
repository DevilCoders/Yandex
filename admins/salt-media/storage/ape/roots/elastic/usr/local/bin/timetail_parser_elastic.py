#!/usr/bin/env python
from __future__ import print_function

import sys


counters = dict()
TIMINGS = dict()


def update_timings(name, value):
    "Update timings key:value pair in globbal dict 'TIMINGS'"
    try:
        _tm = TIMINGS[name]
    except KeyError:
        TIMINGS[name] = {}
        _tm = TIMINGS[name]
    try:
        _tm[value] += 1
    except KeyError:
        _tm[value] = 1


POSITIONS = dict()
INDEXES = dict()


def initialize(arr):
    """
    Define = positions in tskv fields
    Guess that field message not contains user defined tskv log line
    """
    if arr[0] == "tskv":
        idx = 0
        for fld in arr:
            fld = fld.split("=", 1)[0]
            INDEXES[fld] = len(fld) + 1
            POSITIONS[fld] = idx
            idx += 1
    else:
        raise Exception("Bad line")


def v(f, arr):
    return arr[POSITIONS[f]][INDEXES[f]:]


for line in sys.stdin:
    fields = line.split("\t")
    try:
        initialize.initialized
    except:
        try:
            initialize(fields)
            initialize.initialized = 1
        except:
            continue

    _vhost = v("vhost", fields)

    # skipping line if loggiver requests
    if _vhost == '-' \
            or _vhost.endswith(":3132") \
            or _vhost.startswith('localhost'):
        continue

    # skipping line if IPv4 in vhost
    try:
        for x in _vhost.split(":")[0].split("."):
            int(x)
        continue
    except:
        pass
    # skipping line if IPv6 in vhost
    try:
        if len(_vhost[1:-1].split(":")[0:-1]) > 1:
            continue
    except:
        pass

    # skipping line if ping request
    _request = v("request", fields)
    if _request.startswith('/ping'):
        continue

    _method = v('method', fields)
    _bytes_sent = v('bytes_sent', fields)
    _status = v("status", fields)
    _request_time = v("request_time", fields)
    _upstream_response_time = v("upstream_response_time", fields)
    _upstream_cache_status = v("upstream_cache_status", fields)

    # cut tld with ".yandex"
    try:
        if "yandex" not in _vhost:
            vhost = _vhost
        else:
            vhost = _vhost[:_vhost.find("yandex")].strip('.')
    except:
        continue

    # all counters
    try:
        counters['elastic_rps'] += 1
    except KeyError:
        counters['elastic_rps'] = 1

    # rps for get/post
    try:
        counters['elastic_' + _method.lower()] += 1
    except KeyError:
        counters['elastic_' + _method.lower()] = 1


    # create part of counters map
    try:
        upstream_timings = str(sum(float(x) for x in _upstream_response_time.split(",")))
        name = 'upstream_timings_%s' % _method.lower()
        update_timings(name, upstream_timings)
    except:
        pass

    try:
        upstream_timings = str(sum(int(x) for x in _bytes_sent.split(",")))
        name = 'bytes_sent_%s_is_not_timings' % _method.lower()
        update_timings(name, upstream_timings)
    except:
        pass


    vhost_rps = vhost + "__rps"
    vhost_status = vhost + "__" + _status
    vhost_method = vhost + '__' + _method.lower()

    # create 500 = 0 fake counters
    try:
        counters[vhost_status] += 1
    except KeyError:
        vhost_status_500 = vhost + "__500"
        if vhost_status_500 in counters:
            counters[vhost_status] = 1
        else:
            counters[vhost_status_500] = 0
            counters[vhost_status] = 1


for i, v in counters.iteritems():
    print(i, v)

for i, v in TIMINGS.iteritems():
    sys.stdout.write('@'+i)
    for m, c in v.iteritems():
        sys.stdout.write(" {}@{}".format(m, c))
    sys.stdout.write("\n")
