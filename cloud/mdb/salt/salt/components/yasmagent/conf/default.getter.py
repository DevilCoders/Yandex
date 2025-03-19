#!/usr/bin/env python
import json
import os
import socket

ITYPES = "{{ instances }}".split(',')
PRJ_SPLIT_BY_SHARD = {{ salt['pillar.get']('data:yasmagent:prj_split_by_shard', True) | python }}


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


def get_default_tags(itype):
    grains = get_grains()
    dbaas_conf = get_dbaas_conf()
    if dbaas_conf:
        prj = 'none'
        ctype = dbaas_conf.get('cluster_id')
    elif PRJ_SPLIT_BY_SHARD:
        prj = socket.gethostname().split('.')[0][:-1]
        ctype = grains.get('group', '').replace('_', '-')
    else:
        prj = re.search('^.+[^0-9](?=[0-9]+[a-z])', socket.gethostname().split('.')[0]).group(0)
        ctype = grains.get('group', '').replace('_', '-')
    datacenter = grains.get('short_dc', 'nodc')
    if not datacenter:
        datacenter = 'nodc'
    result = '{fqdn}:11003@{itype} a_itype_{itype} a_prj_{prj} a_ctype_{ctype} a_geo_{geo}'.format(
        fqdn=socket.getfqdn(), itype=itype, prj=prj, ctype=ctype, geo=datacenter)
    return result


if __name__ == '__main__':
    print('\n'.join(map(get_default_tags, ITYPES)))
