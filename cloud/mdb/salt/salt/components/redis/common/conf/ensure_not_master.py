#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
Ensure that the local Redis is not a master.
"""
import argparse
import time
import os
import json
import logging
import redis
import sys
import fcntl
import functools
import signal

from contextlib import contextmanager

from redis.exceptions import ResponseError


logging.basicConfig(
    filename='/var/log/redis/ensure_not_master.log',
    level=logging.DEBUG,
    format='%(asctime)s %(levelname)-8s: %(message)s')
LOG = logging.getLogger("ensure_not_master")

SENTINEL_PORT = {{ salt.mdb_redis.get_sentinel_notls_port() }}
REDIS_PORT = {{ salt.mdb_redis.get_redis_notls_port() }}


def get_config_option(option, config_path='/etc/redis/sentinel.conf', index=1):
    """
    Get full value of option from config file.
    """
    string_start = '{option} '.format(option=option)
    with open(config_path) as config:
        for line in config:
            if line.lower().startswith(string_start.lower()):
                stripped = line.split(' ')[index].strip()
                if stripped.startswith('"') and stripped.endswith('"'):
                    return stripped[1:-1]
                return stripped


def redispass():
    """
    Get the local Redis password.
    """
    with open(os.path.expanduser('~/.redispass')) as redispass:
        data = json.load(redispass)
    return data['password']


def failover(connection, master_name):
    """
    Initiate failover.
    """
    failover_cmd = get_config_option(
        'sentinel rename-failover',
        config_path='/etc/redis/sentinel.conf',
        index=3) or 'failover'
    try:
        connection.execute_command('sentinel', failover_cmd, master_name)
    except ResponseError as err:
        str_err = str(err)
        if 'INPROG' in str_err:
            return 'Failover in progress. Waiting...'
        if 'NOGOODSLAVE No suitable replica to promote' in str_err:
            return 'No suitable replica to promote. Waiting...'
        raise
    return 'Triggering failover.'


def cluster_failover(connection):
    """
    Initiate failover on Sharded Redis Cluster.
    """
    failover_cmd = get_config_option(
        'rename-cluster-subcommand FAILOVER',
        config_path='/etc/redis/redis-main.conf',
        index=2) or 'failover'
    connection.execute_command('cluster', failover_cmd)
    return 'Triggering failover.'


def _get_conn(password, host='localhost'):
    return redis.StrictRedis(host=host, port=REDIS_PORT, password=password)


def ensure_not_master(password, attempts):
    """
    Ensure that the local host is not a master.
    """
    was_master_with_slaves = False
    local_conn = _get_conn(password)
    for _ in range(attempts):
        info = local_conn.info()
        LOG.info("ENSURE: redis info = {}, attempt = {}".format(info, _))
        if info['role'] != 'master':
            print('The local Redis is not a master.')
            LOG.info('The local Redis is not a master.')
            return was_master_with_slaves
        elif info['connected_slaves'] == 0:
            print('There are 0 connected slaves.')
            LOG.info('There are 0 connected slaves.')
            return was_master_with_slaves

        was_master_with_slaves = True
        if info['cluster_enabled']:
            slave_ip = info['slave0']['ip']
            slave_conn = _get_conn(password, slave_ip)
            result = cluster_failover(slave_conn)
        else:
            sentinel = redis.StrictRedis(port=SENTINEL_PORT)
            masters = sentinel.sentinel_masters()
            master_name, data = list(masters.items())[0]
            result = failover(sentinel, master_name)
        print(result)
        LOG.info(result)
        time.sleep(2)


def _wait(password, attempts):
    local_conn = _get_conn(password)
    for _ in range(attempts):
        info = local_conn.info()
        LOG.info("WAIT: redis info = {}, attempt = {}".format(info, _))
        if info['role'] == 'slave' and info['master_link_status'] == 'up':
            # node already promoted to slave - we want to get fully functional node before exit
            LOG.info("successfully waited for master switch: slave")
            sys.exit(0)
        if info['role'] == 'master' and info['connected_slaves'] == 0:
            # node is still marked as master - but we already has another as 0 slaves connected
            LOG.info("successfully waited for master switch: master")
            sys.exit(0)
        time.sleep(5)
    LOG.error("failed to wait for master switch")
    sys.exit(1)


def _dry_run(password):
    local_conn = _get_conn(password)
    info = local_conn.info()
    sys.exit(info['role'] == 'master')


def _ensure_not_master(dry_run, wait, attempts):
    password = redispass()
    if dry_run:
        _dry_run(password)
    was_master_with_slaves = ensure_not_master(password, attempts)
    if was_master_with_slaves and wait:
        _wait(password, attempts)


def _get_args():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '-d',
        '--dry-run',
        action="store_true",
        default=False,
        help='Test run feature')
    parser.add_argument(
        '-w',
        '--wait',
        action="store_true",
        default=False,
        help='Wait till master switch ends')
    parser.add_argument(
        '-a',
        '--attempts',
        type=int,
        default=10,
        help='Attempts to get result')
    args = parser.parse_args()
    return args.dry_run, args.wait, args.attempts


if __name__ == '__main__':
    dry_run, wait, attempts = _get_args()
    _ensure_not_master(dry_run, wait, attempts)
