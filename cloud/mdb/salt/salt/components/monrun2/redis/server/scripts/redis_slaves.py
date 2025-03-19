#!/usr/bin/env python
"""
Check replicas` status
"""

import argparse
import os
import sys
import json

from redis import Redis


def die(status=0, message='OK'):
    """
    Print status in monrun-compatible format
    """
    print('{status};{message}'.format(status=status, message=message))
    sys.exit(0)


def check_replication(conn, ha_hosts):
    """
    Check replication lag against warn and crit thresholds
    """
    info = conn.info(section='replication')
    if info.get('role', 'slave') != 'master':
        die()

    connected_slaves = info.get('connected_slaves', 0)
    if ha_hosts and connected_slaves < len(ha_hosts) - 1:
        code = 1 if (connected_slaves or len(ha_hosts) <= 2) else 2
        die(
            code,
            '{connected_slaves} slaves online'.format(
                connected_slaves=connected_slaves),
        )
    die()


def _main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '-p',
        '--port',
        type=int,
        default=6379,
        help='Port to connect to')
    args = parser.parse_args()
    ha_hosts = []
    if os.path.exists('/etc/dbaas.conf'):
        with open('/etc/dbaas.conf') as dbaas_conf:
            ha_hosts = json.load(dbaas_conf)['shard_hosts']

    with open(os.path.expanduser('~/.redispass')) as redispass:
        password = json.load(redispass)['password']
    for address in ['127.0.0.1', '::1']:
        try:
            conn = Redis(
                host=address, port=args.port, db=0, password=password)
            if conn.ping():
                return check_replication(conn, ha_hosts)
        except Exception as exc:
            if str(exc) == 'max number of clients reached':
                die(1, 'conns overlimit')
    die(1, 'redis is dead')


if __name__ == '__main__':
    _main()
