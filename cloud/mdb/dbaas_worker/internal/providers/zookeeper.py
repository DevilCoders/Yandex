"""
Zookeeper interaction module
"""
import os

from kazoo.client import KazooClient
from kazoo.exceptions import KazooException, NoNodeError

from dbaas_common import retry, tracing

from .common import BaseProvider, Change

KAZOO_RETRIES = retry.on_exception(KazooException, factor=10, max_wait=60, max_tries=6)

CLIENT_RETRIES = dict(max_tries=6, delay=0.1, backoff=2, max_jitter=0.8, max_delay=60)


# pylint: disable=too-few-public-methods
class KazooContext:
    """
    KazooClient context helper
    """

    def __init__(self, hosts):
        self.client = KazooClient(
            hosts=hosts, connection_retry=CLIENT_RETRIES, command_retry=CLIENT_RETRIES, timeout=1.0
        )

    def __enter__(self):
        self.client.start()
        return self.client

    def __exit__(self, *_):
        self.client.stop()


def _get_children(zk_client, path):
    try:
        return zk_client.get_children(path)
    except NoNodeError:
        return []  # in the case ZK deletes a znode while we traverse the tree


def _rec_node_delete(zk_client, path, node):
    for subpath in _get_children(zk_client, path):
        if subpath == node:
            zk_client.delete(os.path.join(path, subpath), recursive=True)
        else:
            _rec_node_delete(zk_client, os.path.join(path, subpath), node)


class Zookeeper(BaseProvider):
    """
    Zookeeper provider
    """

    @KAZOO_RETRIES
    @tracing.trace('ZooKeeper Path Absent')
    def _absent(self, zk_hosts, path):
        tracing.set_tag('zookeeper.fqdns', zk_hosts)
        tracing.set_tag('zookeeper.path', path)

        with KazooContext(zk_hosts) as client:
            if client.exists(path):
                client.delete(path, recursive=True)

    def absent(self, zk_hosts, path):
        """
        Make sure that path does not exist in zk
        """
        self.add_change(Change(f'zk.{zk_hosts}.{path}', 'removed'))
        self._absent(zk_hosts, path)
