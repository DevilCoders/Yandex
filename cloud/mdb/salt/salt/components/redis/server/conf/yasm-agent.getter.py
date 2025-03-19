#!/usr/bin/env python
import re
import sys
import json
import socket
import os
import argparse

from redis import StrictRedis

CONDUCTOR_GRAINS_FILE = '/tmp/.grains_conductor.cache'
DBAAS_FILE = '/etc/dbaas.conf'
TIER_CACHE_FILE = '/tmp/.redis_tier.cache'


def read_file_json(filename):
    try:
        with open(filename) as f:
            content = json.loads(f.read())
    except Exception:
        content = {}
    return content


def get_grains():
    return read_file_json('/tmp/.grains_conductor.cache')


def get_dbaas_conf():
    return read_file_json('/etc/dbaas.conf')


def redispass():
    return read_file_json(os.path.expanduser('~/.redispass'))['password']


def get_role():
    conn = StrictRedis(password=redispass())
    info = conn.info('replication')
    if info['role'] == 'master':
        return 'master'
    return 'replica'


def gen_getter(itype):
    grains = get_grains()
    dbaas_conf = get_dbaas_conf()
    prj = 'none'
    group = grains.get('group', 'none').replace('_', '-')
    ctype = dbaas_conf.get('cluster_id', group)
    tier = get_role()
    with open(TIER_CACHE_FILE,'w') as f_cache:
       f_cache.write(tier)

    dc = grains.get('short_dc', 'nodc')
    if not dc:
        switch = grains.get('switch')
        if switch:
            dc = switch[:3]
        else:
            dc = 'nodc'
    res = socket.getfqdn() + ':11003@' + itype + \
        ' a_itype_' + itype + \
        ' a_prj_' + prj + \
        ' a_ctype_' + ctype + \
        ' a_geo_' + dc + \
        ' a_tier_' + tier
    return res


def main():
    print(gen_getter('mdbredis'))


if __name__ == '__main__':
    main()
