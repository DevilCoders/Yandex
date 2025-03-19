#!/usr/bin/python

from kazoo.client import KazooClient, KazooState
from kazoo.handlers.threading import SequentialThreadingHandler
from contextlib import closing
from subprocess import check_output
import argparse
import os.path
from json import loads
from sys import exit

import psycopg2

ZK_TIMEOUT = 3
ZK_HOSTS = '{{ salt['pillar.get']('data:pgsync:zk_hosts', 'zk-df-e2e01f.db.yandex.net:2181,zk-df-e2e01h.db.yandex.net:2181,zk-df-e2e01k.db.yandex.net:2181') }}'

LETTER_TO_DC = {'h': 'sas',
                'e': 'iva',
                'f': 'myt',
                'k': 'vla',
                'i': 'man'}


def get_dc_by_fqdn(fqdn):
    if fqdn[:3] in LETTER_TO_DC.values():
        return fqdn[:3]
    return LETTER_TO_DC[fqdn[fqdn.find(".db") - 1]]


def is_master():
    dsn = 'dbname=postgres user=monitor connect_timeout=1 host=localhost'
    with closing(psycopg2.connect(dsn)) as conn:
        cur = conn.cursor()
        cur.execute('select pg_is_in_recovery() as replica;')
        return not cur.fetchone()[0]


def _create_kazoo_client(hosts, timeout):
    retry_options = {
            'max_tries': 0,
            'delay': 0,
            'backoff': 1,
            'max_jitter': 0,
            'max_delay': 1}

    return KazooClient(
            hosts=hosts,
            handler=SequentialThreadingHandler(),
            timeout=timeout,
            connection_retry=retry_options,
            command_retry=retry_options)


def _get_zk_info(zk, file, cid):
    path = '/pgsync/{cid}/{file}'.format(cid=cid, file=file)
    event = zk.get_async(path)
    event.wait(ZK_TIMEOUT)
    return loads(event.get_nowait()[0])

def _get_all_hosts(zk, cid):
    return zk.get_children('/pgsync/{cid}/all_hosts'.format(cid=cid))

def get_map_application_name2hostname(hostnames):
    result = {}
    to_replace = '.-'
    for host in hostnames:
        appname = host
        for _c in to_replace:
            appname = appname.replace(_c, '_')
        result[appname] = host
    return result

def get_info(from_hosts, from_dc):
    zk = _create_kazoo_client(ZK_HOSTS, ZK_TIMEOUT)
    event = zk.start_async()
    event.wait(ZK_TIMEOUT)

    if os.path.exists('/etc/dbaas.conf'):
        with open('/etc/dbaas.conf', 'r') as file:
            dbaas_conf = loads(file.read())
        cluster_id = dbaas_conf['cluster_id']
        cluster_hosts = dbaas_conf['cluster_nodes']['ha']
    else:
        cluster_id = '{{ salt['pillar.get']('data:pgsync:zk_lockpath_prefix', salt['grains.get']('id').split('.')[0][:-1]) }}'
        if cluster_id.startswith('/pgsync/'):
            cluster_id = cluster_id[len('/pgsync/'):]
        cluster_hosts = _get_all_hosts(zk, cluster_id)

    replics_info = _get_zk_info(zk, 'replics_info', cluster_id)
    app2host = get_map_application_name2hostname(cluster_hosts)

    with open('/tmp/pgsync.status', 'r') as file:
        pgsync_status = loads(file.read())

    master = pgsync_status['zk_state']['lock_holder']
    active_nodes = [app2host.get(node['application_name'], '') for node in replics_info
                    if app2host.get(node['application_name'], '') in cluster_hosts] + [master]

    candidates = []
    for active_node in active_nodes:
        bad_node = False
        for prefix in from_hosts:
            if active_node.startswith(prefix):
                bad_node = True
        for dc in from_dc:
            if get_dc_by_fqdn(active_node) == dc:
                bad_node = True
        if not bad_node:
            candidates.append(active_node)

    candidates_lag = {app2host.get(node['application_name'], ''): node['replay_lag_msec'] / 1000
                      for node in replics_info
                      if app2host.get(node['application_name'], '') in candidates
                      and app2host.get(node['application_name'], '') != master}
    if master in candidates:
        candidates_lag[master] = -1

    return candidates_lag, cluster_hosts


def get_args():
    parser = argparse.ArgumentParser()

    # can't use empty  string as default, because it's prefix of every string
    # no one fqdn starts with ' '
    parser.add_argument('--from-hosts',
                        type=str,
                        default=' ',
                        help="List of prefixes of affected hosts")

    parser.add_argument('--max-lag',
                        type=int,
                        default=240,
                        help="Max replication lag of a candidate for role master")

    parser.add_argument('--wait-duration',
                        type=int,
                        default=300,
                        help="How long wait for switchover to complete, 0s to return immediately (default 5m0s)")

    parser.add_argument('--dry-run',
                        help='Shows if the current host is a master.'
                             ' If it is shows fqdn of a candidate for role master',
                        action='store_true')

    parser.add_argument('--from-dc',
                        type=str,
                        default=' ',
                        help="List of affected DC's")
    parser.add_argument('--run-from-replica',
                        help="Start switchover even if script executed on replica",
                        action="store_true")

    return parser.parse_args()


if __name__ == '__main__':

    args = get_args()

    if not is_master() and not args.run_from_replica:
        print('NO')
        exit(0)

    candidates_lag, ha_nodes = get_info(args.from_hosts.split(','), args.from_dc.split(','))

    if len(candidates_lag) == 0:
        message = 'There is no candidates for role master'
        if args.dry_run:
            print('YES {0}'.format(message))
            exit(0)
        else:
            print(message)
            exit(len(ha_nodes) > 1)

    candidate = min(candidates_lag, key=lambda x: candidates_lag[x])
    lag = candidates_lag[candidate]
    if lag > args.max_lag:
        message = 'Candidate for role master ({0}) has too much lag: {1}'.format(candidate, lag)
        if args.dry_run:
            print('YES {0}'.format(message))
            exit(0)
        else:
            print(message)
            exit(1)

    if lag == -1:
        message = 'No need to switch'
        if args.dry_run:
            print('YES {0}'.format(message))
        else:
            print(message)
        exit(0)

    message = 'new master: {0}'.format(candidate)
    if args.dry_run:
        print('YES {0}'.format(message))
    else:
        print(message)
        cmd = 'pgsync-util switchover -y -b -d {0} -t {1}'.format(candidate, args.wait_duration)
        check_output(cmd.split())
