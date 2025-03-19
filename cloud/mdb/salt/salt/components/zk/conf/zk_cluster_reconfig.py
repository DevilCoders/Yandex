#!/usr/bin/env python3
'''
Reconfig ZooKeeper cluster
'''

import click
import logging
import os
import sys
import json
from kazoo.client import KazooClient

LOG_FILE = '/var/log/yandex/zk_cluster_reconfig.log'

CLIENT_RETRIES = dict(max_tries=6, delay=0.1, backoff=2, max_jitter=0.8, max_delay=60)
PASSWORD_FILE = "~/.zkpass"


def get_super_pass():
    """
    Return password for ZooKeeper superuser
    """
    password = None
    try:
        with open(os.path.expanduser(PASSWORD_FILE)) as superpass:
            password = json.load(superpass)['password']
    except IOError:
        password = None
    return password


@click.command('zookeeper-join')
@click.option('-c', '--zid', type=str, help='ZooKeeper ID', required=True)
@click.option('-f', '--fqdn', type=str, help='ZooKeeper FQDN', required=True)
def zookeeper_join_command(zid, fqdn):
    logging.info('Join ZooKeeper host to cluster, ZID: {zid}, FQDN: {fqdn}'.format(zid=zid, fqdn=fqdn))

    try:
        zk_client = KazooClient(connection_retry=CLIENT_RETRIES, command_retry=CLIENT_RETRIES, timeout=1.0)
        zk_client.start()
        password = get_super_pass()
        if password:
            zk_client.add_auth('digest', 'super:{password}'.format(password=password))
        formatted = 'server.{zid}={fqdn}:2888:3888:participant;[::]:2181'.format(zid=zid, fqdn=fqdn)
        zk_client.reconfig(joining=formatted, leaving=None, new_members=None)
        zk_client.stop()
    except Exception as e:
        zk_client.stop()
        logging.error('Failed to reconfigure ZooKeeper', exc_info=True)
        print(f'Failed to reconfigure ZooKeeper: {repr(e)}', file=sys.stderr)
        sys.exit(1)


@click.command('zookeeper-leave')
@click.option('-c', '--zid', type=str, help='ZooKeeper ID', required=True)
def zookeeper_leave_command(zid):
    logging.info('Leave ZooKeeper host from cluster, ZID: {zid}'.format(zid=zid))

    try:
        zk_client = KazooClient(connection_retry=CLIENT_RETRIES, command_retry=CLIENT_RETRIES, timeout=1.0)
        zk_client.start()
        password = get_super_pass()
        if password:
            zk_client.add_auth('digest', 'super:{password}'.format(password=password))
        zk_client.reconfig(joining=None, leaving=zid, new_members=None)
        zk_client.stop()
    except Exception as e:
        zk_client.stop()
        logging.error('Failed to reconfigure ZooKeeper', exc_info=True)
        print(f'Failed to reconfigure ZooKeeper: {repr(e)}', file=sys.stderr)
        sys.exit(1)


@click.group()
@click.option('--debug', is_flag=True, help='Enable debug output.')
def _main(debug):
    loglevel = 'DEBUG' if debug else 'INFO'
    logging.basicConfig(
        filename=LOG_FILE,
        level=loglevel,
        format='%(asctime)s [%(levelname)s] %(name)s: %(message)s')


_main.add_command(zookeeper_join_command)
_main.add_command(zookeeper_leave_command)


if __name__ == '__main__':
    _main()
