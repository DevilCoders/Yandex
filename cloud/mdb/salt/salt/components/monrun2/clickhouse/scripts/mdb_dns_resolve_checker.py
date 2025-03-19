#!/usr/bin/env python3

import json
import socket
import requests
import sys
import xml.etree.ElementTree as xml
from dns.resolver import query
from tenacity import retry, stop_after_attempt

DBAAS_CONF_PATH = '/etc/dbaas.conf'
NAME_TEMPLATES = {
    'cluster': "c-{cid}.rw.{suffix}",
    'shard': '{shard}.c-{cid}.rw.{suffix}',
}

NON_HA_CHECKS = {
    'cluster': 'SELECT min(hostname) FROM _system.primary_election',
    'shard': 'SELECT hostName()',
}

HA_CHECKS = {
    'cluster': """
        SELECT host_name
        FROM system.clusters
        WHERE cluster = getMacro('cluster')
        ORDER BY shard_num, host_name DESC
     """,
    'shard': """
        SELECT host_name
        FROM system.clusters
        WHERE cluster = getMacro('cluster')
          AND shard_num = (SELECT shard_num FROM system.clusters
                           WHERE (cluster = getMacro('cluster')) AND is_local)
        ORDER BY host_name DESC
    """,
}
ZK_PATHS = {
    'cluster': '/_system/cluster_primary_election',
    'shard': '/_system/shard_primary_election/{shard_name}',
}


def die(status, message):
    """
    Emit status and exit.
    """
    print('%s;%s' % (status, message))
    sys.exit(0)


class ClickhouseClient:

    def __init__(self, host=None):
        port_settings = self._get_port_settings()
        self.host = host if host != None else socket.gethostname()
        if 'https_port' in port_settings:
            self.protocol = 'https'
            self.port = port_settings['https_port']
            self.verify = '/etc/clickhouse-server/ssl/allCAs.pem'
        else:
            self.protocol = 'http'
            self.port = port_settings['http_port']
            self.verify = None

    def execute(self, query):
        params = {}
        if query:
            params['query'] = query

        response = requests.get(
            f'{self.protocol}://{self.host}:{self.port}',
            params=params,
            headers={
                'X-ClickHouse-User': '_monitor',
            },
            timeout=10,
            verify=self.verify)
        response.raise_for_status()
        return response.text.strip()

    @retry(stop=stop_after_attempt(3), reraise=True)
    def ping(self):
        return self.execute(None)

    @classmethod
    def _get_port_settings(self):
        result = {}
        try:
            root = xml.parse('/var/lib/clickhouse/preprocessed_configs/config.xml')
            for setting in ('http_port', 'https_port'):
                node = root.find(setting)
                if node is not None:
                    result[setting] = node.text

        except FileNotFoundError as e:
            die(1, f'clickhouse-server config not found: {e.filename}')

        except Exception as e:
            die(1, f'Failed to parse clickhouse-server config: {e}')

        return result


ch_client = ClickhouseClient()


def get_dbaas_conf():
    """
    Get cluster configuraiton file
    """
    path = DBAAS_CONF_PATH
    return json.load(open(path))


def has_zookeeper(conf):
    for subcluster in conf['cluster']['subclusters'].values():
        if 'zk' in subcluster['roles']:
            return True

    return ch_client.execute(
        """SELECT COUNT(*) = 1 FROM system.tables WHERE database='system' AND name='zookeeper'""") == '1'


def find_first_alive_host(hosts, table_zk_path):
    query = "SELECT count() FROM system.zookeeper WHERE path = '{zk_path}' AND name = 'is_active'"
    for host in hosts.split('\n'):
        zk_path = f"{table_zk_path}/replicas/{host}"
        if ch_client.execute(query.format(zk_path=zk_path))[0] == '1':
            return host


def get_cname(qname, optional=False):
    try:
        dns_response = query(qname, 'cname')  # list of returned names
    except Exception as e:
        if not optional:
            raise e
        dns_response = []

    if len(dns_response) > 1:
        raise RuntimeError("unexpectedly got multiple records for cname: %d" % len(dns_response))
    if not dns_response:
        return ""
    return dns_response[0].to_text()[:-1]


def check():
    dbc = get_dbaas_conf()
    cid = dbc.get('cluster_id')
    shard_name = dbc.get('shard_name')
    ha_check = has_zookeeper(dbc)
    checks = HA_CHECKS if ha_check else NON_HA_CHECKS

    fqdn = socket.getfqdn()
    parts = fqdn.split('.')
    for i in range(len(parts) - 1, 0, -1):
        if parts[i] in ('db', 'mdb'):
            break
    suffix = '.'.join(parts[i:])

    for name, cname_template in NAME_TEMPLATES.items():
        cidname = cname_template.format(shard=shard_name, cid=cid, suffix=suffix)
        err_base_msg = f"cid: '{cid}', fqdn: '{fqdn}', alias: '{cidname}'"

        try:
            cname = get_cname(cidname)
        except Exception as exc:
            die(2, err_base_msg + f", no cname for primary host, err: " + repr(exc))

        if ha_check:
            hosts = ch_client.execute(checks[name])
            expected_primary = find_first_alive_host(hosts, ZK_PATHS[name].format(shard_name=shard_name))
            expected_be_primary = expected_primary == fqdn
        else:
            expected_primary = ch_client.execute(checks[name])
            expected_be_primary = expected_primary == fqdn

        is_primary = cname == fqdn
        if is_primary != expected_be_primary:
            msg = f"{err_base_msg}, wrong primary alias: '{cname}'"
            if expected_primary is not None:
                msg = f"{msg}, expected: '{expected_primary}'"
            die(2, msg)

        if is_primary:
            try:
                ch_client_alias = ClickhouseClient(cidname)
                ch_client_alias.ping()
            except Exception as exc:
                die(2, f"cname ping exception: {repr(exc)}'")

    die(0, "OK")


def _main():
    try:
        ch_client.ping()
        check()
    except Exception as exc:
        die(1, f"exception: {repr(exc)}'")


if __name__ == '__main__':
    _main()
