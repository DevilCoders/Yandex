"""
Zookeeper helpers
"""
from kazoo.client import KazooClient

from tests.helpers import docker, internal_api, workarounds

CLIENT_RETRIES = dict(max_tries=6, delay=0.1, backoff=2, max_jitter=0.8, max_delay=60)


class KazooContext:
    """
    KazooClient context helper
    """

    def __init__(self, hosts):
        self.client = KazooClient(
            hosts=hosts, connection_retry=CLIENT_RETRIES, command_retry=CLIENT_RETRIES, timeout=1.0)

    def __enter__(self):
        self.client.start()
        return self.client

    def __exit__(self, *_):
        self.client.stop()


def get_zk_client(context):
    """
    Get zookeeper client
    """
    host, port = docker.get_exposed_port(
        docker.get_container(context, 'zookeeper01'), context.conf['projects']['zookeeper']['expose']['zk'])

    return KazooClient(f'{host}:{port}')


@workarounds.retry(wait_fixed=1000, stop_max_attempt_number=10)
def get_zk_configuration(context):
    """For some reason the value may be empty from time to time."""

    def parse_configuration(text, secure_port=0):
        result = []
        for line in text.split('\n'):
            if line.startswith('server'):
                val = line.split('=')[1]
                zk_node = {
                    'host': (val.split(':')[0]),
                    'port': '{port}'.format(port=secure_port) if secure_port else val.split(':')[-1],
                }
                if secure_port:
                    zk_node['secure'] = '1'
                result.append(zk_node)
        return result

    path = '/zookeeper/config'
    value = get_zk_value(context, path)
    if not value:
        raise Exception(f'"{path}" value is emtpy')
    #  Set secure_port to 0 when data:unmanaged:enable_zk_tls is not True (see configuration.py)
    return parse_configuration(value, 2281)


def get_zk_value(context, path):
    """Get value from ClickHouse's ZooKeeper."""
    with KazooContext(get_zk_hosts(context)) as client:
        value, _ = client.get(path)
        return value.decode()


def get_zk_hosts(context):
    """Get ClickHouse's ZooKeeper hosts."""
    internal_api.load_cluster_into_context(context)
    hosts = internal_api.build_host_type_dict(context)['ZOOKEEPER']
    hosts_ports = internal_api.get_cluster_ports(context, hosts, port=2181)
    return [f'{host}:{port}' for host, port in hosts_ports]
