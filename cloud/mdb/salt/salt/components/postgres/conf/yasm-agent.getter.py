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

{% set dbaas = salt['pillar.get']('data:dbaas') %}
{%- set is_dbaas_db = salt['pillar.get']('data:dbaas:cluster_id') -%}
{% if is_dbaas_db %}
ctype = {{ dbaas['cluster_id'] | python }}
{% else %}
ctype = {{ salt['pillar.get']('data:yasmagent:ctype', "d.get('group', '')") }}.replace('_', '-')
{% endif %}

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

    cur.execute("SELECT pg_is_in_recovery()")

    if cur.fetchone()[0] is False:
        tier = 'primary'
except Exception:
    pass


def genGetter(itype):
{% if is_dbaas_db %}
    try:
        with open('/etc/dbaas.conf') as dbaas_file:
            dbaas_conf = json.load(dbaas_file)
    except Exception:
        dbaas_conf = {}
    prj = dbaas_conf.get('folder', {}).get('folder_ext_id', 'none')
{% elif salt['pillar.get']('data:yasmagent:prj_split_by_shard', True) %}
    prj = socket.gethostname().split('.')[0][:-1]
{% else %}
    prj = re.search('^.+[^0-9](?=[0-9]+[a-z])', socket.gethostname().split('.')[0]).group(0)
{% endif %}
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
