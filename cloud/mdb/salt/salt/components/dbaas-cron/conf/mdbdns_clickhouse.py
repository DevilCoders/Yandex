"""
MDB DNS updater for ClickHouse.

Update per-cluster and per-shard primary host fqdns in mdb-dns-server.
"""
import itertools

import requests
from tenacity import retry, stop_after_attempt

from mdbdns_upper import MDBDNSUpper, init_logger
from pkg_resources import parse_version

UPPER = None


class MDBDNSUpperClickhouse(MDBDNSUpper):
    def __init__(self, logger, params):
        super(MDBDNSUpperClickhouse, self).__init__(logger, params)
        self._shardid = params["shard_id"]
        self._shard_name = params["shard_name"]
        self._ssl_enabled = params["ssl_enabled"]
        self._has_zookeeper = params["has_zookeeper"]
        self._ch_version = params["ch_version"]

    def check_and_up(self):
        self._logger.debug("detecting cluster primary...")
        if self._is_cluster_primary():
            self.update_dns()

        self._logger.debug("detecting shard primary...")
        if self._is_shard_primary():
            self.update_dns(shardid=self._shardid)

    def _is_cluster_primary(self):
        if self._has_zookeeper:
            table_zk_path = "/_system/cluster_primary_election"
            hosts = self._execute("SELECT host_name FROM system.clusters "
                                  "WHERE cluster = getMacro('cluster') ORDER BY shard_num, host_name DESC")
            return self._find_first_alive_host(hosts, table_zk_path) == self._fqdn

        return self._execute_check("SELECT min(hostname) = hostName() FROM _system.primary_election")

    def _is_shard_primary(self):
        if self._has_zookeeper:
            table_zk_path = f"/_system/shard_primary_election/{self._shard_name}"
            hosts = self._execute("SELECT host_name FROM system.clusters "
                                  "WHERE cluster = getMacro('cluster') "
                                  "AND shard_num = (SELECT shard_num FROM system.clusters "
                                  "WHERE (cluster = getMacro('cluster')) AND is_local) "
                                  "ORDER BY host_name DESC")
            return self._find_first_alive_host(hosts, table_zk_path) == self._fqdn

        return True

    def _find_first_alive_host(self, hosts, table_zk_path):
        query = "SELECT count() FROM system.zookeeper WHERE path = '{zk_path}' AND name = 'is_active'"
        for host in hosts:
            zk_path = f"{table_zk_path}/replicas/{host}"
            is_alive = self._execute_check(query.format(zk_path=zk_path))
            self._logger.debug(f"{host} {'is' if is_alive else 'is not'} alive")
            if is_alive:
                return host

    def _version_lt(self, value):
        return parse_version(self._ch_version) < parse_version(value)

    @retry(stop=stop_after_attempt(3), reraise=True)
    def _execute(self, query):
        if self._ssl_enabled:
            protocol = "https"
            port = 8443
        else:
            protocol = "http"
            port = 8123

        r = requests.get(
            f"{protocol}://{self._fqdn}:{port}",
            params={
                "query": f"{query} FORMAT JSONCompact",
            },
            headers={
                "X-ClickHouse-User": "_dns",
            },
            timeout=10,
            verify=self._ca_path)
        r.raise_for_status()
        return flatten(r.json()['data'])

    def _execute_check(self, query):
        return self._execute(query)[0] in ('1', True)


def mdbdns_clickhouse(params, log_file, log_level='DEBUG', rotate_size=10485760, backup_count=1):
    """
    Run MDB DNS update.
    """
    global UPPER
    if not UPPER:
        log = init_logger(
            name=__name__,
            log_file=log_file,
            log_level=log_level,
            rotate_size=rotate_size,
            backup_count=backup_count)
        log.info("Initialization MDB DNS upper")
        try:
            UPPER = MDBDNSUpperClickhouse(log, params)
        except Exception as exc:
            log.error("failed to init MDB DNS upper failed by exception: %s", repr(exc))
            raise

    try:
        UPPER.check_and_up()
    except Exception as exc:
        UPPER._logger.error("mdbdns upper failed with: %s", repr(exc))
        raise


def flatten(value):
    """
    Flatten list of lists.
    """
    return list(itertools.chain.from_iterable(value))
