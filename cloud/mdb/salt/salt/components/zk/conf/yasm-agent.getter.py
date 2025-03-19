#!/usr/bin/python

import sys
import json
import socket
import os

ITYPE = 'mdbzk'
CONDUCTOR_GRAINS_FILE = '/tmp/.grains_conductor.cache'
TIER_CACHE_FILE = '/tmp/.zookeeper_tier.cache'
DBAAS_FILE = '/etc/dbaas.conf'

DEFAULT_HOST = socket.getfqdn()
DEFAULT_PORT = 2181
DEFAULT_TIMEOUT = 5
DEFAULT_DBNAME = 'admin'
DEFAULT_TIER = 'follower'

COMMAND = 'mntr'
BUFFER_SIZE = 4096


def get_conductor_data():
    try:
        with open(CONDUCTOR_GRAINS_FILE) as cg_fobj:
            conductor = json.loads(cg_fobj.read())
    except Exception:
        conductor = {}

    if os.path.exists(DBAAS_FILE):
        with open(DBAAS_FILE) as dbaas_fobj:
            dbaas = json.loads(dbaas_fobj.read())
        ctype = dbaas.get('cluster_id', 'none')
    else:
        ctype = conductor.get('group', '').replace('_', '-')

    datacenter = conductor.get('short_dc', 'nodc')
    if not datacenter:
        switch = conductor.get('switch')
        if switch:
            datacenter = switch[:3]
        else:
            datacenter = 'nodc'

    return ctype, datacenter


def get_tier(host=None, port=None, timeout=None):
    if host is None:
        host = DEFAULT_HOST
    if port is None:
        port = DEFAULT_PORT
    if timeout is None:
        timeout = DEFAULT_TIMEOUT

    conn = socket.socket(family=socket.AF_INET6)
    conn.settimeout(timeout)
    conn.connect((host, port))

    conn.send(COMMAND)
    data = conn.recv(BUFFER_SIZE)
    conn.close()

    tier = DEFAULT_TIER
    for line in data.splitlines():
        try:
            key, value = line.split()
            if key == 'zk_server_state':
                tier = value
                break
        except ValueError:
            pass

    try:
        with open(TIER_CACHE_FILE, 'w') as f_cache:
            f_cache.write(tier)
    except Exception:
        pass
    return tier


def main():
    try:
        tier = get_tier()
    except Exception:
        tier = DEFAULT_TIER

    try:
        ctype, datacenter = get_conductor_data()
    except Exception:
        sys.exit(1)

    prj = 'none'
    res = DEFAULT_HOST + ':11003@' + ITYPE + \
        ' a_itype_' + ITYPE + \
        ' a_prj_' + prj + \
        ' a_ctype_' + ctype + ' a_geo_' + datacenter + \
        ' a_tier_' + tier

    print(res)


if __name__ == '__main__':
    main()

