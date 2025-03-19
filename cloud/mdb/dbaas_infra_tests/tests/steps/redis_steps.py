"""
Redis related steps
"""
import time

from behave import then, when
from hamcrest import (assert_that, contains_string, equal_to, is_not, starts_with)
from redis import StrictRedis
from redis.exceptions import ResponseError

from tests.helpers import internal_api
from tests.helpers.crypto import gen_plain_random_string
from tests.helpers.docker import get_file, run_command
from tests.helpers.internal_api import get_cluster_ports, get_host_port
from tests.helpers.step_helpers import step_require
from tests.steps.internal_api import find_host

REDIS_CONFIG = "/etc/redis/redis.conf"
# wait (see start-pre.sh in systemd) plus some addon to stabilize Redis
WAIT_PRESTART_ON_CRASH = 30 + 10


def _find_redis_master(context, shard):
    password = context.conf['redis_creds']['password']
    for host in context.hosts:
        if host['shardName'] != shard:
            continue
        hostname, port = get_host_port(context, host, 6379)
        conn = StrictRedis(host=hostname, port=port, db=0, password=password)
        role = conn.info()['role']
        if role == 'master':
            return host['name']
    raise RuntimeError('Failed to find shard master')


@when('we choose one redis master and remember it')
def step_redis_we_chose_one_master_and_remember_it(context):
    shard = context.hosts[0]['shardName']
    master_name = _find_redis_master(context, shard)

    context.state['failover_shard_name'] = shard
    context.state['failover_shard_master'] = master_name


@then('redis master should be different from saved')
def step_redis_master_should_be_different_from_saved(context):
    if 'failover_shard_name' not in context.state:
        raise RuntimeError('Failed to find chosen shard')
    if 'failover_shard_master' not in context.state:
        raise RuntimeError('Failed to find chosen shard master')

    shard = context.state['failover_shard_name']

    old_master_name = context.state['failover_shard_master']
    master_name = _find_redis_master(context, shard)

    assert_that(master_name, is_not(equal_to(old_master_name)), 'Expected redis master to be different from saved')


@when('we attempt to failover redis cluster "{cluster_name}" with saved master')
def step_failover_redis_cluster(context, cluster_name):
    if 'failover_shard_master' not in context.state:
        raise RuntimeError('Failed to find chosen shard master')

    master_name = context.state['failover_shard_master']
    context.execute_steps('''
        When we attempt to start failover on cluster "{cluster_name}" with following parameters
        """
        hostNames: [{host}]
        """
        '''.format(cluster_name=cluster_name, host=master_name))


@when('we insert {count:d} keys into shard "{shard:Param}"')
@step_require('folder', 'hosts', 'cluster_config', 'cluster_type')
def step_redis_insert_into_shard(context, count, shard=None):
    shard_hosts = [host for host in context.hosts if host['shardName'] == shard]
    hostports = get_cluster_ports(context, shard_hosts, port=6379)
    password = context.conf['redis_creds']['password']

    for host, port in hostports:
        conn = StrictRedis(host=host, port=port, db=0, password=password)
        role = conn.info()['role']
        if role != 'master':
            continue
        key = 0
        for _ in range(count):
            while True:
                try:
                    key += 1
                    conn.set(key, gen_plain_random_string(10))
                    break
                except ResponseError as err:
                    if 'MOVED' not in str(err):
                        raise
        return
    raise RuntimeError('Failed to find shard master')


@then('redis major version is "{version}"')
def step_redis_check_version(context, version):
    internal_api.redis_query(context, query="INFO server")
    parsed = [_.split(':')[1] for _ in context.query_result['result'] if 'redis_version:' in _]
    if len(parsed) == 0:
        raise RuntimeError('failed to parse version from INFO server query:', context.query_result)
    host_version = parsed[0]
    assert_that(host_version, starts_with(version), 'Expected redis version = {}, host version = {}'.format(
        version, host_version))


def _mode_to_tls_enabled(mode):
    return mode == 'on'


@then('tls mode is "{mode}"')
def step_redis_check_tls_mode(context, mode):
    tls_enabled = _mode_to_tls_enabled(mode)
    try:
        internal_api.redis_query(context, query="INFO server", tls_enabled=tls_enabled)
    except OSError:
        if tls_enabled:
            raise


@then('config on host in geo "{geo}" contains the following line')
def step_redis_check_config(context, geo, path=REDIS_CONFIG):
    host = find_host(context, geo=geo)
    line = context.text.strip()
    config = get_file(host, path)
    assert_that(config, contains_string(line))


def kill_redis(hostname):
    """
    Kill redis on a specified hostname
    :param hostname:
    :return:
    """
    code, output = run_command(hostname, 'killall redis-server')
    assert code == 0, 'killing redis-server failed, code is {0}, output is "{1}"'.format(code, output)


def get_cluster_shards(hosts):
    """
    Get current cluster shard names list
    :param hosts:
    :return:
    """
    return {host['shardName'] for host in hosts}


def get_cluster_masters(context, shards):
    """
    Get current cluster masters list based on total host list and shard names list
    :param context:
    :param shards:
    :return:
    """
    return [_find_redis_master(context, shard) for shard in shards]


def get_cluster_slaves(hosts, masters):
    """
    Get current cluster slaves list based on total host list and master names list
    :param hosts:
    :param masters:
    :return:
    """
    return [host['name'] for host in hosts if host['name'] not in masters]


@when('we kill redis on {role:Param} hosts')
@step_require('hosts')
def step_kill_redis_in_cluster(context, role=None):
    hosts = context.hosts
    shards = get_cluster_shards(hosts)
    masters = get_cluster_masters(context, shards)
    slaves = get_cluster_slaves(hosts, masters)
    if role == 'master':
        for host in masters:
            kill_redis(host)
            # we can't kill all masters simultaneously in sharded
            # as no slave will be promoted to master till majority of old masters return
            time.sleep(WAIT_PRESTART_ON_CRASH)
    elif role == 'slave':
        for host in slaves:
            kill_redis(host)
    elif role == 'all':
        for host in masters + slaves:
            kill_redis(host)
    else:
        raise Exception('choose one of existing roles or add another with handler')


def restart_redis(hostname, conf_dict=None):
    """
    Restarts Redis instance on given host.
    """
    data = []
    code, output = run_command(hostname, 'service redis-server stop')
    assert code == 0, 'stopping redis-server failed, code is {0}, output is "{1}"'.format(code, output)
    data.append([code, output])

    if conf_dict:
        for name, value in conf_dict.items():
            echo_cmd = 'echo "{} {}"'.format(name, value)
            code, output = run_command(hostname, "bash -c '{} >> {}'".format(echo_cmd, REDIS_CONFIG))
            assert code == 0, 'appending Redis config failed, code is {0}, output is "{1}"'.format(code, output)
            data.append([code, output])

    code, output = run_command(hostname, 'service redis-server start')
    assert code == 0, 'starting redis-server failed, code is {0}, output is "{1}"'.format(code, output)
    data.append([code, output])
    code, output = run_command(hostname, 'cat {}'.format(REDIS_CONFIG))
    data.append([code, output])


@when('we restart redis on all hosts')
@when('we restart redis on all hosts with config names "{names:Param}" and values "{values:Param}"')
@step_require('hosts')
def step_restart_redis_in_cluster(context, names=None, values=None):
    conf_dict = None
    if names and values:
        conf_dict = {}
        for name, value in zip(names.split(','), values.split(',')):
            conf_dict[name] = value
    for host in context.hosts:
        restart_redis(host['name'], conf_dict=conf_dict)


def clear_redis_data(hostname):
    """
    Clears Redis data on given host.
    """
    code, output = run_command(hostname, "bash -c 'rm -rf /var/lib/redis/*'")
    assert code == 0, 'clearing Redis data failed, code is {0}, output is "{1}"'.format(code, output)


@when('we clear Redis data on all hosts')
@step_require('hosts')
def step_clear_redis_data_in_cluster(context):
    for host in context.hosts:
        clear_redis_data(host['name'])


def check_redis_appendonly_missing(hostname):
    """
    Checks Redis aof-file existence on given host.
    """
    code, output = run_command(hostname, "ls /var/lib/redis/appendonly.aof")
    assert code != 0, '{}: aof-file unexpectedly exists, code is {}, output is "{}"'.format(hostname, code, output)


@then('aof missing on all hosts')
@step_require('hosts')
def step_check_aof_missing_in_cluster(context):
    for host in context.hosts:
        check_redis_appendonly_missing(host['name'])


def check_redis_appendonly_exists(hostname):
    """
    Checks Redis aof-file existence on given host.
    """
    code, output = run_command(hostname, "ls /var/lib/redis/appendonly.aof")
    assert code == 0, '{}: aof-file unexpectedly missing, code is {}, output is "{}"'.format(hostname, code, output)


@then('aof exists on all hosts')
@step_require('hosts')
def step_check_aof_exists_in_cluster(context):
    for host in context.hosts:
        check_redis_appendonly_exists(host['name'])
