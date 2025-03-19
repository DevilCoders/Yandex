#!/usr/bin/python

import urllib2
import time
from functools import wraps
import socket
import sys
import json
import argparse
import re

conductor = 'http://c.yandex-team.ru/api-cached'
out_file = '/var/tmp/pre_ping'
TAG = 'dashing-ping'
STRM_DC = re.compile('(?<=strm-)[a-z]+')


def get_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('-g', '--conductor_group', default='')
    args = parser.parse_args()
    return args


def retry(ExceptionToCheck, tries=4, delay=3, backoff=2, logger=None):
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
    :param logger: logger to use. If None, print
    :type logger: logging.Logger instance
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
                    if logger:
                        logger.warning(msg)
                    else:
                        print msg
                    time.sleep(mdelay)
                    mtries -= 1
                    mdelay *= backoff
            return f(*args, **kwargs)

        return f_retry  # true decorator

    return deco_retry


def check_dc(datacenter, hostname):
    if not datacenter:
        dc = STRM_DC.search(hostname)
        if dc:
            datacenter = dc.group(0)
    return datacenter


def main():
    args = get_args()
    args_group = args.conductor_group

    hostname = socket.gethostname()
    c_hosts = http_req("%s/hosts/%s?format=json" % (conductor, hostname))
    c_hosts_json = json.load(c_hosts)
    dc = c_hosts_json[0]['datacenter']
    dc = check_dc(dc, hostname)

    if args_group:
        group = args_group
    else:
        group = c_hosts_json[0]['group']
        c_groups = http_req("%s/hosts2groups/%s?format=json" %
                            (conductor, hostname))
        c_groups_json = json.load(c_groups)
        for x in c_groups_json:
            c_tags = http_req(
                "%s/get_group_tags/%s?recursion=0&format=json" % (conductor, x['name']))
            c_tags_json = json.load(c_tags)
            if TAG in c_tags_json:
                group = x['name']

    all_hosts = http_req(
        "%s/groups2hosts/%s?format=json&fields=fqdn,datacenter_name" % (conductor, group))
    all_hosts_json = json.load(all_hosts)

    out_dcs = {'my_dc': dc}
    for host in all_hosts_json:
        dc = host['datacenter_name']
        fqdn = host['fqdn']
        try:
            out_dcs[dc].append(fqdn)
        except KeyError:
            out_dcs[dc] = [fqdn]
    with open(out_file, 'w') as outfile:
        json.dump(out_dcs, outfile)


@retry(urllib2.URLError, tries=4, delay=3, backoff=2)
def http_req(request):
    try:
        http_answer = urllib2.urlopen(request)
        return http_answer
    except Exception as err:
        print err
        sys.exit(1)


if __name__ == "__main__":
    main()
