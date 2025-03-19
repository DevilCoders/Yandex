#!/usr/bin/env python
# -*- coding: utf-8 -*-

from __future__ import print_function

from functools import wraps
import time
import os
import requests
import re
import argparse
import socket

HOSTNAME = socket.gethostname()


def retry(ExceptionToCheck, tries=4, delay=3, backoff=2, out_function=None):
    """Retry calling the decorated function using an exponential backoff.

    http://www.saltycrane.com/blog/2009/11/trying-out-retry-decorator-python/
    original from: http://wiki.python.org/moin/PythonDecoratorLibrary#Retry

    :param ExceptionToCheck: the exception to check. may be a tuple of
        exceptions to check
    :type ExceptionToCheck: Exception or tuple
    :param tries: number of times to try (not retry) before giving up
    :type tries: int
    :param delay: initial delay between retries in seconds
    :type delay: int
    :param backoff: backoff multiplier e.g. value of 2 will double the delay
        each retry
    :type backoff: int
    :param out_function: function to use.
    """
    def deco_retry(f):

        @wraps(f)
        def f_retry(*args, **kwargs):
            mtries, mdelay = tries, delay
            while mtries > 1:
                try:
                    return f(*args, **kwargs)
                except ExceptionToCheck as e:
                    msg = "%s, Retrying in %d seconds..." % (str(e), mdelay)
                    if out_function:
                        out_function(msg)
                    time.sleep(mdelay)
                    mtries -= 1
                    mdelay *= backoff
            return f(*args, **kwargs)

        return f_retry  # true decorator

    return deco_retry


@retry((requests.exceptions.RequestException), tries=3, delay=2, backoff=2)
def http_req_try(request, connect_timeout=1, read_timeout=3, verify=True):
    http_answer = requests.get(request, timeout=(connect_timeout, read_timeout), verify=verify)
    http_answer.raise_for_status()
    return http_answer


def get_root_datacenter(host, base_url="http://c.yandex-team.ru/api-cached/hosts/{host}?format=json", cache_time=3600):
    """
    [
      {
        "admins": [
        ],
        "description": "",
        "short_name": "strm-kiv12.strm",
        "root_datacenter": "kiv",
        "datacenter": "kiv",
        "fqdn": "strm-kiv12.strm.yandex.net",
        "group": "strm-stream-kiv",
        "id": 3318307
      }
    ]
    """

    dc = None
    cache_dc = '/var/tmp/host_dc-{}'.format(host)
    if os.path.isfile(cache_dc) and os.path.getctime(cache_dc) + cache_time > time.time():
        with open(cache_dc, 'r') as f:
            dc = f.read().strip()
    else:
        url = base_url.format(host=host)
        h = http_req_try(url, verify=False)
        dc = h.json()[0]['root_datacenter']
        if dc:
            with open(cache_dc, 'w') as f:
                f.writelines(dc)

    return dc


def get_last_requests():
    """
    {
      "last_requests": [
        1529586875,
        1529586880,
        1529586880,
        1529586880,
        1529586885,
        1529586885,
        1529586890,
        1529586890,
        1529586895,
        1529586895
      ]
    }
    """

    try:
        url = 'http://localhost:11003/last_requests/'
        h = http_req_try(url, verify=False)
        last_requests = h.json()
    except Exception:
        last_requests = {'last_requests': [5, 10]}

    return last_requests


def check(last_requests, dc):
    monrun_msg = []
    code = 0

    old_ts = last_requests[0]

    diffs = []
    for ts in last_requests:
        diff = ts - old_ts
        diffs.append(diff)
        old_ts = ts

    if max(diffs) > 10:
        monrun_msg.append('Not monitored')
        code = 2

    if dc:
        monrun_msg.append('Dc {}'.format(dc))
    else:
        monrun_msg.append('Datacenter not found')
        code = 2

    monrun_out = "{};{}".format(code, ';'.join(monrun_msg))
    print(monrun_out)


def main():
    try:
        last_requests = get_last_requests()
        dc = get_root_datacenter(HOSTNAME)

        check(last_requests['last_requests'], dc)
    except Exception as e:
        print("2;{}".format(e))


if __name__ == '__main__':
    main()
