#!/usr/bin/env python

import json
import os
import socket
import subprocess
import sys

import porto

try:
    import psycopg2
except ImportError:
    pass


def get_pg_tier(root, name):
    tier = 'unknown'
    try:
        os.environ['PGPASSFILE'] = \
            '{path}/home/monitor/.pgpass'.format(path=root)
        con = psycopg2.connect(
            ('host={host} port=5432 dbname=postgres '
             'user=monitor connect_timeout=1').format(host=name))
        cur = con.cursor()
        cur.execute('SELECT pg_is_in_recovery()')
        if cur.fetchone()[0]:
            tier = 'replica'
        else:
            tier = 'primary'
    except Exception:
        pass

    return tier


def get_mongodb_tier(root, name):
    tier = 'unknown'
    try:
        with open(os.path.join(root, 'tmp/.mongo_tier.cache')) as f:
            tier = f.read()
    except Exception:
        pass

    return tier


def get_redis_tier(root, name):
    tier = 'unknown'
    try:
        with open(os.path.join(root, 'tmp/.redis_tier.cache')) as f:
            tier = f.read()
    except Exception:
        pass

    return tier


def get_mysql_tier(root, name):
    default_role = 'unknown'
    try:
        with open(os.path.join(root, 'tmp/.mysql_state.cache')) as f:
            state = json.load(f)
        return state.get('role', default_role).replace('master', 'primary')
    except Exception:
        return default_role


def get_clickhouse_tier(root, name):
    return "master"


def get_zookeeper_tier(root, name):
    tier = 'unknown'
    try:
        with open(os.path.join(root, 'tmp/.zookeeper_tier.cache')) as f:
            tier = f.read()
    except Exception:
        pass

    return tier


def get_external_yasmagent_suppressed(name, conn):
    container = conn.Find(name)
    root = container.GetProperty('root')
    yasm_suppress_file = '{path}/etc/yandex/suppress-external-yasmagent'.format(path=root)
    return os.path.exists(yasm_suppress_file)


def get_container_tags(itype, geo, name, conn, default_ctype):
    container = conn.Find(name)

    abs_name = container.GetProperty('absolute_name')
    prj = name.split('.')[0][:-1]
    ctype = default_ctype
    root = container.GetProperty('root')
    tier = None

    dbaas_file = '{path}/etc/dbaas.conf'.format(path=root)
    if os.path.exists(dbaas_file):
        with open(dbaas_file) as data:
            dbaas = json.load(data)
        if dbaas.get('shard_id', None):
            prj = str(dbaas['shard_id']).lower()
        else:
            prj = dbaas.get('folder', {}).get('folder_ext_id', 'none')
        ctype = dbaas['cluster_id']
        cluster_type = dbaas.get('cluster_type', None)
        if cluster_type == 'mongodb_cluster':
            tier = get_mongodb_tier(root, name)
        elif cluster_type == 'redis_cluster':
            tier = get_redis_tier(root, name)
        elif cluster_type == 'mysql_cluster':
            tier = get_mysql_tier(root, name)
        elif cluster_type == 'clickhouse_cluster':
            if dbaas.get('subcluster_name', None) == 'zookeeper_subcluster':
                tier = get_zookeeper_tier(root, name)
            else:
                tier = get_clickhouse_tier(root, name)
        elif cluster_type == 'zookeeper_cluster':
            tier = get_zookeeper_tier(root, name)
        else:
            tier = get_pg_tier(root, name)
    if tier is None:
        tier = get_pg_tier(root, name)

    result = ('{name} portocontainer={portoname} '
              'a_itype_{itype} a_prj_{prj} '
              'a_ctype_{ctype} a_geo_{geo} '
              'a_tier_{tier}').format(
                  name=name,
                  portoname=abs_name,
                  itype=itype,
                  prj=prj,
                  ctype=ctype,
                  geo=geo,
                  tier=tier)
    return result


def get():
    try:
        with open('/tmp/.grains_conductor.cache') as conductor_cache:
            data = json.load(conductor_cache)
    except Exception:
        data = {}

    hostname = socket.getfqdn()
    itype = 'mdbdom0'
    geo = data.get('short_dc', 'nodc')
    if not geo:
        geo = data.get('switch')[:3]

    if '.mail.yandex.net' in hostname:
        default_ctype = 'pers'
    elif '-pgaas-' in hostname and '.db.yandex.net' in hostname:
        default_ctype = 'pgaas'
    else:
        default_ctype = 'test'

    conn = porto.Connection()
    conn.connect()
    for name in conn.List():
        if conn.GetData(name, 'parent') != '/':
            continue
        if get_external_yasmagent_suppressed(name, conn):
            continue
        yield get_container_tags(itype, geo, name, conn, default_ctype)


if __name__ == '__main__':
    if os.getuid() != 0:
        subprocess.call(['sudo', sys.argv[0]])
    else:
        for res in get():
            print(res)
