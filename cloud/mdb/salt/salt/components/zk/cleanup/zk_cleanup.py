#!/usr/bin/env python3
"""
Cleanup ZooKeeper after removing ClickHouse host from cluster
"""

import click
import logging
import os
import sys
import json
from kazoo.client import KazooClient
from kazoo.exceptions import NoNodeError, NotEmptyError

LOG_FILE = '/var/log/yandex/zk_cleanup.log'

CLIENT_RETRIES = dict(max_tries=6, delay=0.1, backoff=2, max_jitter=0.8, max_delay=60)
PASSWORD_FILE = "~/.zkpass"


def get_children(zk_client, path):
    try:
        return zk_client.get_children(path)
    except NoNodeError:
        return []  # in the case ZK deletes a znode while we traverse the tree


def node_delete(zk_client, node):
    try:
        zk_client.delete(node, recursive=True)
    except NoNodeError:
        #  Someone deleted node before us. Do nothing.
        logging.warning('Node {node} is already absent, skipped'.format(node=node))
    except NotEmptyError:
        #  Someone created child node while deleting.
        #  I'm not sure that we can get this exception with recursive=True.
        #  Do nothing.
        logging.warning('Node {node} is not empty, skipped'.format(node=node))


def rec_node_delete(zk_client, path, nodes):
    for subpath in get_children(zk_client, path):
        if subpath in nodes:
            node_delete(zk_client, os.path.join(path, subpath))
        else:
            rec_node_delete(zk_client, os.path.join(path, subpath), nodes)


def rec_find_parents(zk_client, path, nodes, parent):
    paths = set()
    (current_parent, current_node) = os.path.split(path)
    for subpath in get_children(zk_client, path):
        if subpath in nodes and current_node == parent:
            paths.add(current_parent)
        else:
            paths = paths.union(rec_find_parents(zk_client, os.path.join(path, subpath), nodes, parent))
    return paths


def nodes_absent(zk_client, path, nodes):
    if zk_client.exists(path):
        rec_node_delete(zk_client, path, nodes)


def find_parents(zk_client, path, nodes, parent_name):
    """
    Find all nodes with name 'parent_name' with subnode 'node_name' (one of 'nodes')
    Example:
      params:
        path = '/clickhouse/mdb1234567890abcdef
        nodes = ['iva-fedcba0987654321.db.yandex.net']
        parent_name = 'replicas'
      nodes in ZK:
        /clickhouse/mdb1234567890abcdef/mdb_system/write_sli_part/shard1/replicas/iva-fedcba0987654321.db.yandex.net
        /clickhouse/mdb1234567890abcdef/mdb_system/write_sli_part/shard1/replicas/myt-abcdefghijklmnop.db.yandex.net
      result:
        [
            '/clickhouse/mdb1234567890abcdef/mdb_system/write_sli_part/shard1'
        ]
    """
    paths = set()
    if zk_client.exists(path):
        paths = paths.union(rec_find_parents(zk_client, path, nodes, parent_name))
    return list(paths)


def set_replicas_is_lost(zk_client, paths, replica_names):
    """
    Set flag <path>/replicas/<replica_name>/is_lost to 1
    """
    for path in paths:
        replicas_path = os.path.join(path, 'replicas')
        if zk_client.exists(replicas_path):
            for replica_name in replica_names:
                replica_path = os.path.join(replicas_path, replica_name)
                if zk_client.exists(replica_path):
                    is_lost_path = os.path.join(replica_path, 'is_lost')
                    logging.info('Set flag {path} to 1'.format(path=is_lost_path))
                    try:
                        zk_client.set(is_lost_path, b'1')
                    except NoNodeError:
                        logging.warning(f'Path is already deleted: {is_lost_path}')


def remove_replicas_queues(zk_client, paths, replica_names):
    """
    Remove <path>/replicas/<replica_name>/queue
    """
    for path in paths:
        replicas_path = os.path.join(path, 'replicas')
        if zk_client.exists(replicas_path):
            for replica_name in replica_names:
                replica_path = os.path.join(replicas_path, replica_name)
                if zk_client.exists(replica_path):
                    queue_path = os.path.join(replica_path, 'queue')
                    logging.info('Remove queue {path}'.format(path=queue_path))
                    node_delete(zk_client, queue_path)


def absent_if_empty_child(zk_client, path, child):
    """
    Remove node if subnode is empty
    """
    if zk_client.exists(path):
        subnodes = get_children(zk_client, os.path.join(path, child))
        if len(subnodes) == 0:
            logging.info('{path} removed'.format(path=path))
            zk_client.delete(path, recursive=True)


def clean_zk_nodes(zk_client, root, nodes):
    """
    Remove zk nodes after host removing
    """
    zk_path = root
    table_paths = find_parents(zk_client, zk_path, nodes, 'replicas')
    set_replicas_is_lost(zk_client, table_paths, nodes)
    remove_replicas_queues(zk_client, table_paths, nodes)
    nodes_absent(zk_client, zk_path, nodes)
    for path in table_paths:
        absent_if_empty_child(zk_client, path, 'replicas')


def get_credentials():
    """
    Return password for ZooKeeper superuser
    """
    try:
        with open(os.path.expanduser(PASSWORD_FILE)) as superpass:
            credentials = json.load(superpass)
            user = credentials.get('user', 'super')
            password = credentials['password']

            return f'{user}:{password}'
    except IOError:
        return None


def connect_zk():
    try:
        zk_client = KazooClient(connection_retry=CLIENT_RETRIES, command_retry=CLIENT_RETRIES, timeout=1.0)
        zk_client.start()

        credentials = get_credentials()
        if credentials:
            zk_client.add_auth('digest', credentials)

        return zk_client
    except Exception as e:
        logging.error(f'Could not connect to ZooKeeper: {repr(e)}')
        return None


def failure_exit(zk_client, error):
    logging.error(repr(error))
    zk_client.stop()
    sys.exit(1)


@click.command('clickhouse-hosts')
@click.option('-c', '--root', type=str, help='Cluster ZooKeeper root path', required=True)
@click.option('-f', '--fqdn', type=str, help='Removed FQDNs, comma separated', required=True)
def clickhouse_hosts_command(root, fqdn):
    logging.info(f'Cleanup after remove ClickHouse from root: {root}, FQDNs: {fqdn}')

    zk_client = connect_zk()
    if not zk_client:
        return

    try:
        hosts = fqdn.split(',')
        clean_zk_nodes(zk_client, root, hosts)
    except Exception as e:
        failure_exit(zk_client, e)

    zk_client.stop()
    logging.info('Cleanup finished')


@click.command('zookeeper-nodes')
@click.option('-p', '--path', type=str, help='Paths for remove, comma separated', required=True)
def zookeeper_nodes_command(path):
    logging.info('Clean nodes: {path}'.format(path=path))

    zk_client = connect_zk()
    if not zk_client:
        return

    try:
        nodes = path.split(',')
        for node in nodes:
            node_delete(zk_client, node)
    except Exception as e:
        failure_exit(zk_client, e)

    zk_client.stop()
    logging.info('Cleanup finished')


@click.group()
@click.option('--debug', is_flag=True, help='Enable debug output.')
def _main(debug):
    loglevel = 'DEBUG' if debug else 'INFO'
    logging.basicConfig(filename=LOG_FILE, level=loglevel, format='%(asctime)s [%(levelname)s] %(name)s: %(message)s')


_main.add_command(clickhouse_hosts_command)
_main.add_command(zookeeper_nodes_command)

if __name__ == '__main__':
    _main()
