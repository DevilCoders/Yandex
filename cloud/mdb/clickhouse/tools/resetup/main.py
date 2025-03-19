#!/usr/bin/env python3
import argparse
import json
import logging
import os
import socket
import subprocess

from cloud.mdb.clickhouse.tools.common.clickhouse import ClickhouseClient

CLIENT_RETRIES = dict(max_tries=6, delay=0.1, backoff=2, max_jitter=0.8, max_delay=60)

GET_USER_TABLES_SQL = """
    SELECT name
    FROM system.tables
    WHERE database <> 'system'
"""


def parse_args():
    """
    Parse command-line arguments.
    """
    parser = argparse.ArgumentParser()
    parser.add_argument('--host', default=socket.getfqdn(), help='')
    parser.add_argument('--port', default=8443, help='Port for connection to local ClickHouse')
    parser.add_argument('--tables', default='', help='Comma-separated list of tables to restore')
    parser.add_argument('--service-manager', choices=['systemd', 'supervisord'], default='systemd')
    parser.add_argument(
        '--insecure',
        action='store_true',
        default=False,
        help='Disable certificate verification for ClickHouse requests.',
    )
    parser.add_argument('-v', '--verbose', action='store_true', default=False, help='Verbose mode.')
    parser.add_argument('--zk-root', default='/clickhouse', help='zk path to clean metadata')

    return parser.parse_args()


def get_dbaas_conf():
    """
    Load /etc/dbaas.conf.
    """
    with open('/etc/dbaas.conf', 'r') as fd:
        return json.load(fd)


def check_no_user_tables(ch_client):
    """
    Check restoring replica doesn't have user tables.
    """
    logging.debug('Checking restoring replica has no user tables')
    result = ch_client.query(GET_USER_TABLES_SQL)
    if result['data']:
        raise RuntimeError('Restoring replica should have no user tables')


def get_ch_shard_hosts(conf, target_host):
    """
    Get shard of target host, all hosts belong to this shard and Zookeeper hosts.
    """
    ch_shard_hosts = []
    shard_name = None
    for subcid, sub_conf in conf['cluster']['subclusters'].items():
        if 'clickhouse_cluster' in sub_conf['roles']:
            for shard_id, sh_conf in sub_conf['shards'].items():
                if target_host in sh_conf['hosts']:
                    ch_shard_hosts.extend(sh_conf['hosts'])
                    shard_name = sh_conf['name']
                    break
    return shard_name, ch_shard_hosts


def restore_schema(src_host, target_port, insecure):
    """
    Restore schema from another host via ch-backup tool.
    """

    cmd = ['ch-backup', '--port', str(target_port), '--protocol', 'https']
    if insecure:
        cmd.append('--insecure')
    cmd += ['restore-schema', '--source-host', src_host, '--exclude-dbs', 'system']
    logging.debug(f'Running: {cmd}')

    # Workaround if locale is not set.
    env = None
    if not os.environ.get('LC_ALL'):
        env = {'LC_ALL': 'en_US.UTF-8'}

    try:
        output = subprocess.check_output(cmd, env=env, stderr=subprocess.STDOUT)
    except subprocess.CalledProcessError as exc:
        output = str(exc.output).replace('\\n', '\n    ')
        logging.exception(f'{cmd} failed: {output} {exc.returncode}')
        raise
    else:
        return output


def main():
    args = parse_args()

    logging.getLogger('kazoo').setLevel('WARNING')
    if args.verbose:
        logging.basicConfig(level='DEBUG', format='%(message)s')

    target_host = args.host
    conf = get_dbaas_conf()

    cid = conf['cluster_id']
    shard_name, ch_shard_hosts = get_ch_shard_hosts(conf, target_host)
    if not ch_shard_hosts:
        raise RuntimeError('Target host is not found among ClickHouse shards')

    logging.debug('Cluster id: %s', cid)
    logging.debug('Recovering host: %s', target_host)
    logging.debug('ClickHouse shard hosts: %s', ch_shard_hosts)

    # Select a host to take metadata from
    for h in ch_shard_hosts:
        if h != target_host:
            src_host = h
            break
    else:
        raise RuntimeError('No source host available')

    check_no_user_tables(ClickhouseClient(host=target_host, port=args.port, insecure=args.insecure))
    restore_schema(src_host, args.port, args.insecure)


if __name__ == '__main__':
    main()
