#!/usr/bin/env python

import re
import sys
import json
import socket
import os
import psycopg2

itypes = "{{ instances }}".split(',')

try:
    with open('/tmp/.grains_conductor.cache') as f:
        d = json.loads(''.join(f.readlines()))
except Exception:
    d = {}

ctype = {{ salt['pillar.get']('data:yasmagent:ctype', "d.get('group', '')") }}.replace('_', '-')

tier = 'replica'

try:
    with open(os.path.expanduser("~/.pgpass")) as pgpass:
        for line in pgpass:
            tokens = line.rstrip().split(':')
            if tokens[3] == 'monitor':
                password = tokens[4]
                break

    conn = psycopg2.connect('host=localhost port=5432 dbname=postgres ' +
                            'user=%s password=%s ' % ('monitor', password) +
                            'connect_timeout=1')
    cur = conn.cursor()

    cur.execute("show transaction_read_only;")

    if 'off' in str(cur.fetchone()[0]):
        tier = 'primary'
except Exception:
    pass

def genGetter(itype):
    prj = re.search('^.+[^0-9](?=[0-9]+[a-z])', socket.gethostname().split('.')[0]).group(0)
    datacenter = d.get('short_dc', 'nodc')
    if not datacenter:
        switch = d.get('switch')
        if switch:
            datacenter = switch[:3]
        else:
            datacenter = 'nodc'
    res = socket.getfqdn() + ':11003@' + itype + \
        ' a_itype_' + itype + \
        ' a_prj_' + prj + \
        ' a_ctype_' + ctype + ' a_geo_' + datacenter + \
        ' a_tier_' + tier
    return res

print '\n'.join(map(genGetter, itypes))
