"""
MDB DNS updater for Redis

Update primary host fqdn in cluster in mdb-dns-server
"""
from mdbdns_upper import MDBDNSUpper, init_logger
import json
import socket
from redis import StrictRedis
from rediscluster import RedisCluster

UPPER = None
MY_STATE_PATH = '/tmp/.redis_state.cache'
DEF_STATE = {'role': 'replica'}
TLS = {{ salt.mdb_redis.tls_enabled() }}
CLUSTER_PORT = {{ salt.mdb_redis.get_redis_replication_port() }}
SENTINEL_PORT = {{ salt.mdb_redis.get_sentinel_notls_port() }}
REDIS_PORT = {{ salt.mdb_redis.get_redis_notls_port() }}
MONITOR_REDISPASS = {{ salt.mdb_redis.get_redispass_file('monitor') | yaml_squote }}


def get_ip_from_addrinfo(ipaddr):
    return ipaddr[-1][0]


def is_master_in_my_addrinfos(addrinfos, master):
    for addrinfo in addrinfos:
        if master['host'] == get_ip_from_addrinfo(addrinfo):
            return True


def get_state_sentinel(master_name):
    state = dict(DEF_STATE)
    sentinel = StrictRedis(port=SENTINEL_PORT)
    # this returns IPv6 address
    ipaddrs = socket.getaddrinfo(socket.getfqdn(), REDIS_PORT)
    for ipaddr in ipaddrs:
        if sentinel.sentinel_master(master_name).get('ip') == get_ip_from_addrinfo(ipaddr):
            state['role'] = sentinel.sentinel_master(master_name).get('role-reported')
            break
    return state


def get_cluster_nodes():
    startup_nodes = [{"host": "127.0.0.1", "port": str(CLUSTER_PORT)}]
    with open(MONITOR_REDISPASS) as redispass:
        password = json.load(redispass)['password']
    kwargs = dict(startup_nodes=startup_nodes, password=password, decode_responses=True, skip_full_coverage_check=True,
        port=CLUSTER_PORT,
    )
    if TLS:
        kwargs.update(dict(ssl=TLS, ssl_cert_reqs=None))
    local = RedisCluster(**kwargs)
    return local.cluster_nodes()


def get_cluster_masters():
    cluster_nodes = get_cluster_nodes()
    return [node for node in cluster_nodes if node['master'] is None]


def get_first_shard_hosts(shards):
    first_shard = shards[sorted(iter(shards))[0]]
    return first_shard.get('hosts', {})


def get_state_redis_cluster(shards):
    # checks whether this host is from first shard in dbaas.conf
    # and whether it is considered a master by scan
    state = dict(DEF_STATE)
    my_fqdn = socket.getfqdn()
    first_shard_hosts = get_first_shard_hosts(shards)
    if my_fqdn not in first_shard_hosts:
        return state

    my_addrinfos = socket.getaddrinfo(my_fqdn, REDIS_PORT)
    cluster_masters = get_cluster_masters()
    for master in cluster_masters:
        check = is_master_in_my_addrinfos(my_addrinfos, master)
        if check:
            state['role'] = 'master'
            break
    return state


class MDBDNSUpperRedis(MDBDNSUpper):

    def get_state(self):
        """
        Get last fresh redis state
        """
        # The commented lines are what should be (but they won't work because of NOAUTH)
        with open('/etc/dbaas.conf') as dbaas_conf:
            conf = json.load(dbaas_conf)
        shards = conf['cluster']['subclusters'][conf['subcluster_id']].get('shards', {})
        if len(shards) <= 1:
            state = get_state_sentinel(conf['cluster_name'])
        else:
            state = get_state_redis_cluster(shards)
        with open(MY_STATE_PATH, 'w') as fobj:
            json.dump(state, fobj)
        return state

    def check_and_up(self):
        self._logger.debug("checking...")
        dbc = self.get_dbaas_conf()

        cid = dbc.get("cluster_id")
        if not cid or cid != self._cid:
            self._logger.error("invalid cid in dbaas conf")
            return

        state = self.get_state()
        if not state:
            self._logger.warning("invalid or unfresh state")
            return

        if state.get("role") != "master":
            self._logger.info("not a master host")
            return

        self.update_dns()


def mdbdns_redis(log_file, rotate_size, params, backup_count=1):
    """
    Run mdb DNS update
    """
    global UPPER
    if not UPPER:
        log = init_logger(__name__, log_file, rotate_size, backup_count)
        log.info("Initialization MDB DNS upper")
        try:
            UPPER = MDBDNSUpperRedis(log, params)
        except Exception as exc:
            log.error("failed to init MDB DNS upper failed by exception: %s", repr(exc))
            raise exc

    try:
        # TODO: only one shard should update common cluster records
        UPPER.check_and_up()
    except Exception as exc:
        UPPER._logger.error("mdbdns upper failed by exception: %s", repr(exc))
        raise exc
