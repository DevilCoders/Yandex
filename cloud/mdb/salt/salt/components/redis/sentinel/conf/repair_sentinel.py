#!/usr/bin/env python
"""
Redirect Sentinel to look at the right master.
Rediscover the hosts if the config is outdated.
Fix cluster if split-brain occurred.
"""
import grp
import json
import logging
import os
import pwd
import shutil
import socket
import subprocess
import time

from redis import StrictRedis
from redis.exceptions import BusyLoadingError
from redis.exceptions import ConnectionError as RedisConnectionError
from redis.exceptions import ResponseError

logging.basicConfig(level=logging.DEBUG,
                    format='%(asctime)s %(levelname)-8s: %(message)s')
LOG = logging.getLogger()

CONF_NAME = 'repair.conf'
SENTINEL_CONF = '/etc/redis/sentinel.conf'
CONF_BACKUP = '/etc/redis/sentinel.conf.backup'

WD_STOP_FLAG = '{{ salt.mdb_redis.get_sentinel_stopped_flag() }}'

REDIS_STOPPED_FLAG = '{{ salt.mdb_redis.get_redis_server_stop_statefile() }}'
REDIS_RUNNING_FLAG = '{{ salt.mdb_redis.get_redis_pidfile() }}'
REDIS_STARTING_FLAG = '{{ salt.mdb_redis.get_redis_starting_flag() }}'
REDIS_STOPPING_FLAG = '{{ salt.mdb_redis.get_redis_stopping_flag() }}'


def get_connection(host, password=None, port=None, attempts=10):
    """
    Get Redis connection.
    """
    for _ in range(attempts):
        try:
            conn = StrictRedis(host=host, port=port, password=password)
            conn.ping()
            return conn
        except BusyLoadingError:
            time.sleep(5)
        except RedisConnectionError:
            LOG.error('Failed to connect to %s:%s', host, port)
            time.sleep(1)
    return None


def find_masters(ha_hosts, password, redis_port=None):
    """
    Iterate through the hosts list, return dict of masters and their replication stats.
    Example:
    {
        'example-fqdn.db.yandex.net': {
            'master_replid': '...',
            'repl_backlog_size': '...',
            ...
        },
    }
    """
    raise_msg = ""
    masters = dict()
    loading_masters = dict()
    for host in ha_hosts:
        conn = get_connection(host=host, password=password, port=redis_port)
        if not conn:
            msg = 'Failed to connect to {}'.format(host)
            LOG.error(msg)
            raise_msg += "{}\n".format(msg)
            continue
        info = conn.info('replication')
        if info['role'] == 'master':
            loading = conn.info('persistence')['loading']
            (loading_masters if loading else masters)[host] = info
    if not masters:
        if len(loading_masters) == 0:
            msg = 'No masters available to connect in cluster.'
            LOG.error(msg)
            raise RuntimeError(msg)
        masters = loading_masters
        if len(loading_masters) == 1:
            LOG.warning('Master is loading db, not fully functional.')
        else:
            LOG.critical('Several masters loading db detected - going to try to fix it.')

    if len(masters) > 1:
        masters_list = str(masters.keys())
        LOG.critical('Splitbrain occurred. Masters: %s', masters_list)

    if raise_msg:
        # some hosts are unavailable, won't proceed as we could break something in a non-fully functional cluster
        raise RuntimeError(raise_msg)
    return masters


def get_ip(fqdn=None):
    """
    Translate the fqdn to IP address.
    Tries IPv4 first, then IPv6.
    If fqdn is None, returns the IP of the current host.
    """
    if fqdn is None:
        fqdn = socket.gethostname()
    try:
        return socket.gethostbyname(fqdn)
    except Exception:
        LOG.debug('Unable to resolve "%s" to IPv4 address', fqdn)
    try:
        return socket.getaddrinfo(fqdn, None, socket.AF_INET6)[0][4][0]
    except Exception:
        LOG.debug('Unable to resolve "%s" to IPv6 address', fqdn)
    return fqdn


def chown(path, user, group):
    """
    Change the owner of a file.
    """
    LOG.debug('Changing owner of %s to %s:%s', path, user, group)
    uid = pwd.getpwnam(user).pw_uid
    gid = grp.getgrnam(group).gr_gid
    os.chown(path, uid, gid)


def is_valid_config(config_lines, port):
    """
    Check that config has all the required lines.
    """
    required = [
        'daemonize yes', 'port {}'.format(port), 'protected-mode no',
        'sentinel set-disable yes', 'dir /var/lib/redis',
    ]

    return all(
        any(line in cl.replace('"', '') for cl in config_lines)
        for line in required)


def regenerate_sentinel_config():
    """
    Run sentinel config regeneration state
    """
    state_cmd = [
        'timeout',
        '15m',
        'salt-call',
        '--log-file-level=debug',
        'state.sls',
        'components.redis.sentinel.configs.sentinel-config',
        'concurrent=True',
        'pillar={sentinel-restart: true}',  # to avoid fgrep-based master-name validation in state
    ]
    LOG.info('Running salt state: "%s"', ' '.join(state_cmd))
    with open(os.devnull, 'w') as devnull:
        subprocess.check_call(state_cmd,
                              preexec_fn=os.setsid,
                              stdout=devnull,
                              stderr=devnull)
    LOG.info('Finished.')


def stop_sentinel(unlink_wd=True):
    """
    Stop sentinel via service command
    """
    stop_cmd = ['service', 'redis-sentinel', 'stop']
    LOG.info('Stopping the Sentinel.')
    subprocess.check_call(stop_cmd)
    # So we will come again if we fail after this point
    if unlink_wd and os.path.exists(WD_STOP_FLAG):
        os.unlink(WD_STOP_FLAG)


def start_sentinel():
    """
    Start sentinel via service command
    """
    start_cmd = ['service', 'redis-sentinel', 'start']
    LOG.info('Starting the Sentinel.')
    subprocess.check_call(start_cmd)


def restart_sentinel():
    """
    Restart sentinel via service command
    """
    stop_sentinel()
    start_sentinel()


def get_sentinel_config(config_path=SENTINEL_CONF, ignore_hosts=True):
    """
    Read Sentinel config and split it into lines.
    """
    ignore_prefixes = tuple()
    if ignore_hosts:
        ignore_prefixes = tuple(
            'sentinel {}'.format(s)
            for s in ['monitor', 'known-replica', 'known-sentinel', 'auth-pass'])

    config_lines = []
    with open(config_path) as conf:
        for line in conf:
            stripped = line.strip()
            if stripped and not stripped.startswith(ignore_prefixes):
                line = stripped + '\n'
                if line not in config_lines:
                    config_lines.append(line)

    return config_lines


def redirect_local_sentinel(master_fqdn,
                            master_name,
                            password,
                            quorum=2,
                            redis_port=None,
                            master_ip=None):
    """
    Restart the local Sentinel.
    Start monitoring the new master (master_fqdn).
    """
    stop_sentinel()
    LOG.info('Saving Sentinel config backup.')
    shutil.copyfile(SENTINEL_CONF, CONF_BACKUP)

    master_ip = master_ip or get_ip(master_fqdn)
    config_lines = [
        '# Rewritten by repair_sentinel.py\n',
        'sentinel monitor {} {} {} {}\n'.format(
            master_name, master_ip, redis_port, quorum),
        'sentinel auth-pass {} "{}"\n'.format(
            master_name, password),
    ] + get_sentinel_config()

    LOG.info('Rewriting Sentinel config.')
    with open(SENTINEL_CONF, 'w') as conf:
        conf.writelines(config_lines)

    chown(SENTINEL_CONF, 'redis', 'redis')
    chown(CONF_BACKUP, 'redis', 'redis')
    start_sentinel()


def forget_defunct_hosts(sentinel_conn, ha_hosts, master_name, reset_cmd):
    """
    If the Sentinel contains redundant hosts in it's state,
    execute the RESET command.
    """
    sentinels = 0
    replicas = 0
    for line in get_sentinel_config(ignore_hosts=False):
        l = line.strip()
        if l.startswith('sentinel known-replica') and not l.endswith('127.0.0.1 0'):  # bug in redis
            replicas += 1
        if l.startswith('sentinel known-sentinel'):
            sentinels += 1
    must_be = len(ha_hosts) - 1
    if replicas > must_be or sentinels > must_be:
        LOG.info(
            'Sentinel config contains redundant hosts: '
            '%d replicas, %d other sentinels.', replicas, sentinels)
        LOG.info('Executing SENTINEL RESET.')
        sentinel_conn.execute_command('sentinel', reset_cmd, master_name)


def get_config():
    """
    Get the script config.
    """
    local_dir = os.path.realpath(
        os.path.join(os.getcwd(), os.path.dirname(__file__)))
    config_path = os.path.join(local_dir, CONF_NAME)

    with open(os.path.expanduser('~/.redispass')) as redispass:
        data = json.load(redispass)

    try:
        with open(config_path) as conf:
            data.update(json.load(conf))
    except Exception as exc:
        LOG.error('Failed to load the configuration file "%s": %s',
                  config_path, repr(exc))

    return data


def get_fqdn(ip_address):
    """
    Get the fqdn by the ip address.
    """
    try:
        return socket.gethostbyaddr(ip_address)[0]
    except socket.error:
        LOG.error('Failed to resolve %s', ip_address)
        raise


def is_ha_host(ip_address, ha_hosts):
    """
    Check if the ip belongs to the cluster host.
    """
    return get_fqdn(ip_address) in ha_hosts


def check_quorum(sentinel, master_name):
    """
    Check if the current Sentinel configuration is able to reach the quorum.
    """
    try:
        sentinel.execute_command('sentinel ckquorum', master_name)
        return True
    except ResponseError as err:
        LOG.error(str(err))
        return False


def make_redis_slaveof(password, redis_port, slaveof_cmd, config_cmd, master, replication_port):
    conn = get_connection('localhost', password=password, port=redis_port)
    res = conn.execute_command(slaveof_cmd, master, replication_port)
    LOG.debug("switching master result: {}".format(res))
    res = conn.execute_command(config_cmd, 'rewrite')
    LOG.debug("rewriting config result: {}".format(res))


def find_and_repair_master(ha_hosts, password, redis_port, info, local_sentinel, master_name,
                           slaveof_cmd, config_cmd, replication_port, quorum):
    # Handling the split-brain.
    if info['role-reported-time'] < info['failover-timeout']:
        LOG.debug('master reported is too fresh ({}s with {}s failover timeout), waiting for stabilization...'.format(
            info['role-reported-time'] / 1000, info['failover-timeout'] / 1000
        ))
    masters = find_masters(ha_hosts, password, redis_port)
    # Refresh local sentinel info (may be from old epoch)
    config_epoch_before = info['config-epoch']
    info = local_sentinel.sentinel_master(master_name)
    if config_epoch_before != info['config-epoch']:
        LOG.debug("Sentinel managed to change something while we were connecting to Redis - we don't want to interfere")
        return None, None

    if len(masters) == 0:
        msg = 'No masters available to connect in cluster.'
        LOG.error(msg)
        raise RuntimeError(msg)

    if len(masters) == 1:
        return masters.keys()[0], info

    master_ip = info['ip']
    sentinel_master = get_fqdn(master_ip)
    local_fqdn = socket.gethostname()
    if local_fqdn not in masters:
        return None, None
    if local_fqdn != sentinel_master and check_quorum(
            local_sentinel, master_name) and not info['is_sdown']:
        LOG.critical(
            'The local master is lost. Turning it to a slave of %s',
            sentinel_master)
        make_redis_slaveof(password, redis_port, slaveof_cmd, config_cmd, sentinel_master, replication_port)
        redirect_local_sentinel(None, master_name, password, quorum, replication_port, master_ip)

    return None, None


def skip():
    if os.path.exists(REDIS_STARTING_FLAG):
        LOG.debug("redis-server starting...")
        return True
    if os.path.exists(REDIS_STOPPING_FLAG):
        LOG.debug("redis-server stopping...")
        return True
    if not os.path.exists(REDIS_RUNNING_FLAG) and not os.path.exists(REDIS_STOPPED_FLAG):
        LOG.debug("redis-server is not in (starting, stopping, running, stopped) - assuming initial state...")
        return True
    return False


def repair():
    """
    Repair the local Sentinel.
    """
    if skip():
        LOG.debug('skipping')
        return

    config = get_config()
    if not config:
        return
    master_name = config['master_name']
    ha_hosts = sorted(config['ha_hosts'])
    password = config['password']
    redis_port = config.get('redis_port')
    replication_port = config.get('replication_port')
    sentinel_port = config.get('sentinel_port')
    failover_timeout = config.get('failover_timeout', 30000)
    quorum = config.get('quorum', 2)
    reset_cmd = config.get('reset_cmd', 'reset')
    slaveof_cmd = config.get('slaveof_cmd', 'slaveof')
    config_cmd = config.get('config_cmd', 'config')

    local_sentinel = get_connection(host='localhost',
                                    port=sentinel_port,
                                    attempts=1)

    if not is_valid_config(get_sentinel_config(), sentinel_port):
        LOG.error('Invalid Sentinel config.')
        stop_sentinel()
        regenerate_sentinel_config()
        start_sentinel()
        return

    if not local_sentinel:
        if not os.path.exists(WD_STOP_FLAG):
            LOG.error('Local Sentinel is down.')
            restart_sentinel()
        return

    info = local_sentinel.sentinel_master(master_name)
    if info['is_sdown'] and info['role-reported'] == 'master':
        # Master is available, but Sentinel can't connect to it
        master_ip = info['ip']
        master_conn = get_connection(master_ip, password, redis_port)
        if master_conn and master_conn.info('replication')['role'] == 'master':
            LOG.info(
                'Master is available, but marked as "sdown". Restarting the Sentinel with config rewrite.'
            )
            redirect_local_sentinel(None, master_name, password, quorum, replication_port, master_ip=master_ip)
            return
        if not master_conn and len(ha_hosts) == 1:
            LOG.error('Invalid Sentinel config for single-noded cluster.')
            master_fqdn = socket.gethostname()
            LOG.info('Redirecting Sentinel to %s', master_fqdn)
            redirect_local_sentinel(master_fqdn, master_name, password, quorum, replication_port)
            return

    forget_defunct_hosts(local_sentinel, ha_hosts, master_name, reset_cmd)

    # All the things below are related to multi-host clusters.
    if len(ha_hosts) < 2:
        return

    master, info = find_and_repair_master(ha_hosts, password, redis_port, info, local_sentinel, master_name,
                                          slaveof_cmd, config_cmd, replication_port, quorum)

    if master is None:
        return

    # Redis Servers and Sentinels must be out of sync
    if not is_ha_host(info['ip'], ha_hosts) or \
            info['is_sdown'] and \
            info['role-reported'] == 'slave' and \
            info.get('s-down-time', 0) > failover_timeout:
        LOG.critical('Sentinel is monitoring the wrong host: %s', info['ip'])
        LOG.info('Redirecting Sentinel to %s', master)
        redirect_local_sentinel(master, master_name, password, quorum, replication_port)
        return


if __name__ == '__main__':
    repair()
