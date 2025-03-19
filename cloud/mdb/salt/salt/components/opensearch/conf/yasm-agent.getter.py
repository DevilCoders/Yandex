#!/usr/bin/python
import socket
import sys
import json
import os

ITYPE = 'mdbelasticsearch'
CONDUCTOR_GRAINS_FILE = '/tmp/.grains_conductor.cache'
DBAAS_FILE = '/etc/dbaas.conf'


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
        sctype = dbaas.get('subcluster_name', 'datanode_subcluster')
    else:
        ctype = conductor.get('group', '').replace('_', '-')
        prj = 'none'
        sctype = 'datanode_subcluster'

    if prj is None:
        prj = 'none'

    if prj == 'none':
        if sctype == 'datanode_subcluster':
            prj = 'datanode'
        elif sctype == 'masternode_subcluster':
            prj = 'masternode'

    datacenter = conductor.get('short_dc', 'nodc')
    if not datacenter:
        switch = conductor.get('switch')
        if switch:
            datacenter = switch[:3]
        else:
            datacenter = 'nodc'

    return ctype, datacenter, prj


def main():
    try:
        ctype, datacenter, prj = get_conductor_data()
    except Exception:
        sys.exit(1)

    tier = 'replica'

    res = socket.getfqdn() + ':11003@' + ITYPE + \
        ' a_itype_' + ITYPE + \
        ' a_prj_' + prj + \
        ' a_ctype_' + ctype + ' a_geo_' + datacenter + \
        ' a_tier_' + tier

    print(res)


if __name__ == '__main__':
    main()

