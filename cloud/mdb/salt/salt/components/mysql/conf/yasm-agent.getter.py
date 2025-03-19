#!/usr/bin/env python
import json
import os
import socket

import MySQLdb


MYCNF_PATH = os.path.expanduser('~monitor/.my.cnf')
MYSYNC_INFO_PATH = '/var/run/mysync/mysync.info'

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


def get_role():
    conn = None
    try:
        conn = MySQLdb.connect(read_default_file=MYCNF_PATH, db='mysql')
        cur = conn.cursor()
        cur.execute('SHOW SLAVE STATUS')
        status = cur.fetchone()
        if status:
            return 'replica'
        else:
            localhost = get_dbaas_conf().get('fqdn')
            return 'primary' if read_file_json(MYSYNC_INFO_PATH).get('master') == localhost else 'replica'
    except Exception:
        pass
    finally:
        if conn:
            conn.close()
    return 'replica'


def gen_getter(itype):
    grains = get_grains()
    dbaas_conf = get_dbaas_conf()
    prj = dbaas_conf.get('folder', {}).get('folder_ext_id', 'none')
    ctype = dbaas_conf.get('cluster_id')
    tier = get_role()
    datacenter = grains.get('short_dc', 'nodc')
    if not datacenter:
        switch = grains.get('switch')
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

if __name__ == '__main__':
    print(gen_getter('mdb_mysql'))
