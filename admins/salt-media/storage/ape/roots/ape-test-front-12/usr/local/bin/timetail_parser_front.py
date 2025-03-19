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
            INDEXES[fld] = len(fld)+1
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

    if _vhost == '-' or _vhost.endswith(":3132"):
        continue

    try:
        for x in _vhost.split(":")[0].split("."):
            int(x)
        continue
    except:
        pass
    try:
        if len(_vhost[1:-1].split(":")[0:-1]) > 1:
            continue
    except:
        pass

    _request = v("request", fields)
    if _request.startswith('/ping'):
        continue

    _status = v("status", fields)
    _ssl_handshake_time = v("ssl_handshake_time", fields)
    _request_time = v("request_time", fields)
    _upstream_response_time = v("upstream_response_time", fields)
    _upstream_cache_status = v("upstream_cache_status", fields)

    #cut tld with ".yandex"
    try:
        if "yandex" not in _vhost:
            vhost = _vhost
        else:
            vhost = _vhost[:_vhost.find("yandex")].strip('.')
    except:
        continue

    if vhost == "" or vhost == "ya.ru":
        vhost = "l7"

    try:
        tld = _vhost.split('.')[-1].split(':')[0]
    except:
        tld = ""

    if _upstream_cache_status == "-":
        upstream_cache_status = "NOCACHE"
    else:
        upstream_cache_status = _upstream_cache_status

    #all front counters
    status = "front__" + _status
    cache_status = "front__" + upstream_cache_status

    for metric_name in "front__rps",status,cache_status:
        try:
            counters[metric_name] += 1
        except:
            counters[metric_name] = 1

    #create custom vhost api.browser with handle
    if vhost.find("api.browser") != -1:
        vhost = "api_browser"
        try:
            handle = _request.split('/')[1].split('?')[0]
            handle = handle.replace(":", "")
        except:
            handle = ""
        #rewrite
        if handle == "suggest-rich":
            handle = "suggest"
        elif handle == "update-info":
            handle = "updateinfo"
        elif handle == "uma_proto":
            handle = "umaproto"
        elif handle == "search-samples":
            handle = "sample_search"
        elif handle == "sl":
            handle = "smartlinks"
        elif handle == "distrib" or handle == "mcfg" or handle == "configs":
            handle = "brconfigs"
        elif handle == "page-ins-data" or handle == "dashboard-suggest":
            handle = "uproxy"
        #create part of counters map
        try:
            upstream_timings = str(sum(float(x) for x in _upstream_response_time.split(",")))
            vhost_handle_upstream_timings = vhost + "__" + handle + "__upstream_timings"
            update_timings(vhost_handle_upstream_timings, upstream_timings)
        except:
            pass
        vhost_handle_rps = vhost + "__" + handle + "__rps"
        vhost_handle_tld_rps = vhost + "__" + handle + "__" + tld + "__rps"
        vhost_handle_status = vhost + "__" + handle + "__" + _status
        vhost_handle_tld_status = vhost + "__" + handle + "__" + tld + "__" + _status
        vhost_handle_request_timings = vhost + "__" + handle + "__request_timings"
        vhost_handle_cache_status = vhost + "__" + handle + "__" + upstream_cache_status
        vhost_handle_tld_cache_status = vhost + "__" + handle + "__" + tld + "__" + upstream_cache_status

        vhost_rps = vhost + "__rps"
        vhost_tld_rps = vhost + "__" + tld + "__rps"
        vhost_status = vhost + "__" + _status
        vhost_tld_status = vhost + "__" + tld + "__" + _status
        vhost_cache_status = vhost + "__" + upstream_cache_status
        vhost_tld_cache_status = vhost + "__" + tld + "__" + upstream_cache_status

        for metric_name in vhost_rps, vhost_tld_rps, vhost_handle_tld_rps, \
                           vhost_handle_rps, vhost_tld_status, vhost_cache_status, \
                           vhost_tld_cache_status, vhost_handle_tld_status, \
                           vhost_handle_cache_status, vhost_handle_tld_cache_status:
            try:
                counters[metric_name] += 1
            except KeyError:
                counters[metric_name] = 1
        #create 500 = 0 fake counters
        try:
            counters[vhost_status] += 1
        except KeyError:
            vhost_status_500 = vhost + "__500"
            if vhost_status_500 in counters:
                counters[vhost_status] = 1
            else:
                counters[vhost_status_500] = 0
                counters[vhost_status] = 1
        try:
            counters[vhost_handle_status] += 1
        except:
            vhost_handle_status_500 = vhost + "__" + handle + "__500"
            if vhost_handle_status_500 in counters:
                counters[vhost_handle_status] = 1
            else:
                counters[vhost_handle_status_500] = 0
                counters[vhost_handle_status] = 1
        update_timings(vhost_handle_request_timings, _request_time)
        if _ssl_handshake_time != "-":
            vhost_tld_ssl_timings = vhost + "__" + tld + "__ssl_timings"
            update_timings(vhost_tld_ssl_timings, _ssl_handshake_time)
    #create all vhost counters
    else:
        try:
            upstream_timings = str(sum(float(x) for x in _upstream_response_time.split(",")))
            vhost_upstream_timings = vhost + "__upstream_timings"
            update_timings(vhost_upstream_timings, upstream_timings)
        except:
            pass
        vhost_tld_rps = vhost + "__" + tld + "__rps"
        vhost_rps = vhost + "__rps"
        vhost_status = vhost + "__" + _status
        vhost_tld_status = vhost + "__" + tld + "__" + _status
        vhost_request_timings = vhost + "__request_timings"
        vhost_cache_status = vhost + "__" + upstream_cache_status
        vhost_tld_cache_status = vhost + "__" + tld + "__" + upstream_cache_status
        for metric_name in vhost_rps,vhost_tld_rps,vhost_tld_status,\
                           vhost_cache_status,vhost_tld_cache_status:
            try:
                counters[metric_name] += 1
            except:
                counters[metric_name] = 1
        # fake 500 = 0 counters
        try:
            counters[vhost_status] += 1
        except:
            vhost_status_500 = vhost + "__500"
            if vhost_status_500 in counters:
                counters[vhost_status] = 1
            else:
                counters[vhost_status_500] = 0
                counters[vhost_status] = 1

        update_timings(vhost_request_timings, _request_time)

        if _ssl_handshake_time != "-":
            vhost_ssl_timings = vhost + "__ssl_timings"
            update_timings(vhost_ssl_timings, _ssl_handshake_time)
        #counters with handle
        if vhost == "yavision" or vhost == "l7" or \
                vhost == "apefront.tst12.ape" or vhost == "front.tst.ape" or vhost == "docviewer.ape":
            try:
                    handle = _request.split('/')[1].split('?')[0]
            except:
                    handle = ""

            if (handle == "set" or handle == "kullan" or handle == "ya_browser" or \
                    handle == "yandex_browser") and vhost == "l7" and "/s/" in _request:
                try:
                    #/set/s/rsya-tag-users -> rsya-tag-users
                    handle = _request.split('?')[0].split('/')[3]
                except:
                    pass

            if handle != "":
                handle = handle.replace(":", "")
                try:
                    upstream_timings = str(sum(float(x) for x in _upstream_response_time.split(",")))
                    vhost_handle_upstream_timings = vhost + "__" + handle + "__upstream_timings"
                    update_timings(vhost_handle_upstream_timings, upstream_timings)
                except:
                    pass

                vhost_handle_rps = vhost + "__" + handle + "__rps"
                vhost_handle_status = vhost + "__" + handle + "__" + _status
                vhost_handle_request_timings = vhost + "__" + handle + "__request_timings"
                try:
                    counters[vhost_handle_rps] += 1
                except:
                    counters[vhost_handle_rps] = 1

                # fake 500 = 0 counters
                try:
                    counters[vhost_handle_status] += 1
                except:
                    vhost_handle_status_500 = vhost + "__" + handle + "__500"
                    if vhost_handle_status_500 in counters:
                        counters[vhost_handle_status] = 1
                    else:
                        counters[vhost_handle_status_500] = 0
                        counters[vhost_handle_status] = 1

                update_timings(vhost_handle_request_timings, _request_time)

for i, v in counters.iteritems():
    print(i, v)

for i, v in TIMINGS.iteritems():
    sys.stdout.write('@'+i)
    for m, c in v.iteritems():
        sys.stdout.write(" {}@{}".format(m, c))
    sys.stdout.write("\n")
    #print(i, " ".join("{}@{}".format(t, c) for t, c in v.iteritems()))
