#!/usr/bin/python

import sys
import json
import socket
import os
import pymongo

ITYPE = 'mdbmongodb'
CONDUCTOR_GRAINS_FILE = '/tmp/.grains_conductor.cache'
MONGO_TIER_CACHE_FILE = '/tmp/.mongo_tier.cache'
MONGO_TIER_CACHE_FILE_2 = '/tmp/.{}_tier.cache'
PASSFILE = '/home/monitor/.mongopass'
DBAAS_FILE = '/etc/dbaas.conf'
CA_CERTS_FILE = '/opt/yandex/allCAs.pem'

MONGO_ELECTION_TIMEOUT = 1000

MONGO_DEFAULT_HOST = socket.getfqdn()
MONGO_DEFAULT_DBNAME = 'admin'
MONGO_DEFAULT_TIER = 'secondary'
MONGO_SERVICE = '{{ srv }}'
SERVICE_TO_PORT = {
    'mongos': 27017,
    'mongod': 27018,
    'mongocfg': 27019,
}


def get_port():
    if MONGO_SERVICE == '':
        return SERVICE_TO_PORT['mongod']
    else:
        return SERVICE_TO_PORT[MONGO_SERVICE]


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
        prj = dbaas.get('shard_id', 'none')
        sctype = dbaas.get('subcluster_name', 'mongod_subcluster')
    else:
        ctype = conductor.get('group', '').replace('_', '-')
        prj = 'none'
        sctype = 'mongod_subcluster'

    if prj is None:
        prj = 'none'

    if prj == 'none':
        if MONGO_SERVICE == 'mongocfg':
            prj = 'config'
        elif MONGO_SERVICE != '':
            prj = MONGO_SERVICE
        elif sctype == 'mongocfg_subcluster':
            prj = 'config'
        elif sctype == 'mongos_subcluster':
            prj = 'mongos'

    datacenter = conductor.get('short_dc', 'nodc')
    if not datacenter:
        switch = conductor.get('switch')
        if switch:
            datacenter = switch[:3]
        else:
            datacenter = 'nodc'

    return ctype, datacenter, prj


def get_connection_data():
    port = get_port()
    try:
        with open(PASSFILE) as mongopass:
            for line in mongopass:
                fline = line.split(':')
                if int(fline[1]) == port:
                    return fline[0:3]
    except Exception:
        pass

    return MONGO_DEFAULT_HOST, port, MONGO_DEFAULT_DBNAME


def get_tier(host=None, port=None, dbname=None):
    if host is None:
        host = MONGO_DEFAULT_HOST
    if port is None:
        port = get_port()
    if dbname is None:
        dbname = MONGO_DEFAULT_DBNAME

    connect_uri = 'mongodb://{host}:{port}/{dbname}'.format(
        host=host, port=port, dbname=dbname)

    conn = pymongo.MongoClient(
        connect_uri,
        serverSelectionTimeoutMS=MONGO_ELECTION_TIMEOUT,
        ssl=True,
        ssl_ca_certs=CA_CERTS_FILE)

    result = 'primary'\
        if conn[dbname].command('ismaster')['ismaster'] else 'secondary'

    conn.close()
    with open(MONGO_TIER_CACHE_FILE, 'w') as f_cache:
        f_cache.write(result)
    with open(MONGO_TIER_CACHE_FILE_2.format(MONGO_SERVICE), 'w') as f_cache:
        f_cache.write(result)
    return result


def main():
    host, port, dbname = get_connection_data()
    try:
        tier = get_tier(host=host, port=port, dbname=dbname)
    except Exception:
        tier = MONGO_DEFAULT_TIER

    try:
        ctype, datacenter, prj = get_conductor_data()
    except Exception:
        sys.exit(1)

    res = host + ':11003@' + ITYPE + \
        ' a_itype_' + ITYPE + \
        ' a_prj_' + prj + \
        ' a_ctype_' + ctype + ' a_geo_' + datacenter + \
        ' a_tier_' + tier

    print(res)


if __name__ == '__main__':
    main()
