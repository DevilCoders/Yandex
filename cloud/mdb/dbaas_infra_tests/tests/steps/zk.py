"""
Steps related to Zookeeper
"""
from collections import defaultdict

import xmltodict
from behave import then, when
from hamcrest import (assert_that, equal_to, less_than_or_equal_to, none, not_none)
from kazoo.client import KazooClient
from kazoo.exceptions import NoAuthError, NoNodeError

from tests.helpers import zk
from tests.helpers.docker import get_file, run_command
from tests.helpers.internal_api import clickhouse_query
from tests.helpers.matchers import is_subset_of
from tests.helpers.step_helpers import get_step_data


@then('zookeeper has value "{value}" for key "{key}"')
def zk_has_value(context, key, value):
    """
    Modify postgresql cascade replica
    """
    zkclient = zk.get_zk_client(context)
    try:
        zkclient.start()
        zk_value = None
        if zkclient.exists(key):
            val = zkclient.get(key)[0]
            zk_value = val.decode()
    finally:
        zkclient.stop()
    assert_that(zk_value, equal_to(value))


@then('zookeeper has clickhouse node "{key}"')
def zk_has_ch_node(context, key):
    """
    Check that ZooKeeper has a node 'key' in /clickhouse/<cluster>/
    """
    zkclient = KazooClient(zk.get_zk_hosts(context))
    try:
        zkclient.start()
        nodes = zkclient.get_children('/clickhouse')
        assert_that(len(nodes), equal_to(1))
        cluster = nodes[0]
        assert_that(zkclient.exists('/clickhouse/' + cluster + '/' + key), not_none())
    finally:
        zkclient.stop()


@then('zookeeper has not clickhouse node "{key}"')
def zk_has_not_ch_node(context, key):
    """
    Check that ZooKeeper has not a node 'key' in /clickhouse/<cluster>/
    """
    zkclient = KazooClient(zk.get_zk_hosts(context))
    try:
        zkclient.start()
        nodes = zkclient.get_children('/clickhouse')
        assert_that(len(nodes), less_than_or_equal_to(1))
        if len(nodes) == 1:
            cluster = nodes[0]
            assert_that(zkclient.exists('/clickhouse/' + cluster + '/' + key), none())
    finally:
        zkclient.stop()


@then('ZooKeeper consists of following nodes')
def step_consists(context):
    """
    Validate what nodes ZooKeeper consists of.
    """
    expected_geos = defaultdict(lambda: 0)
    geos = defaultdict(lambda: 0)
    for geo in get_step_data(context):
        expected_geos[geo] += 1
    configuration = zk.get_zk_configuration(context)
    for host in configuration:
        geo = host['host'].split('-')[0]
        geos[geo] += 1
    assert_that(geos, equal_to(expected_geos))


@then('ClickHouse is aware of ZooKeeper configuration')
def step_clickhouse_is_aware(context):
    """
    Validate that ClickHouse's knowledge of ZooKeeper is up to date.
    """
    path = "/etc/clickhouse-server/cluster.xml"
    ch_hosts = [h for h in context.hosts if h['type'] == 'CLICKHOUSE']
    zk_configuration = zk.get_zk_configuration(context)
    params = {'yandex': {'zookeeper-servers': {'node': zk_configuration}}}
    for host in ch_hosts:
        contents = get_file(host['name'], path)
        cluster_config = xmltodict.parse(contents)
        assert_that(params, is_subset_of(cluster_config))


@then("ClickHouse can access ZooKeeper")
def step_clickhouse_can_access(context):
    """
    Validate that ClickHouse can access ZooKeeper.
    """
    query = "SELECT path FROM system.zookeeper WHERE path = '/clickhouse'"
    responses = clickhouse_query(context, query=query)

    for response in responses:
        assert_that(
            str(response['value']), equal_to('/clickhouse'), '{0} returned unexpected result: {1}'.format(
                response['host'], response['value']))


@then('ClickHouse Keeper is HA')
def step_clickhouse_keeper_is_ha(context):
    """
    Validate that ClickHouse keeper config is located on 3 nodes.
    """
    assert_that(3, equal_to(get_keepers_count(context)))


def get_keepers_count(context):
    """
    Get number of Clickhouse hosts with Keeper config.
    """
    path = "/etc/clickhouse-server/config.xml"
    ch_hosts = [h for h in context.hosts if h['type'] == 'CLICKHOUSE']
    keeper_count = 0
    for host in ch_hosts:
        config_file = get_file(host['name'], path)
        host_config = xmltodict.parse(config_file)
        if "keeper_server" in host_config['yandex']:
            keeper_count += 1
    return keeper_count


def restart_zookeeper(hostname):
    """
    Restarts ZooKeeper instance on given host.
    """
    run_command(hostname, 'service zookeeper restart')


@when('we restart zookeeper on all hosts')
def step_restart_zookeeper_in_cluster(context):
    hosts = [host for host in context.hosts if host['type'] == 'ZOOKEEPER']
    for host in hosts:
        restart_zookeeper(host['name'])


@then('ACL is "{enabled}"')
def step_check_zk_acl(context, enabled):
    """Check that ACL is working or not."""
    zkclient = KazooClient(zk.get_zk_hosts(context))

    try:
        zkclient.start()
        nodes = zkclient.get_children('/clickhouse')
        assert_that(len(nodes), equal_to(1))
        cluster = nodes[0]
        #  Try to request node without auth credentials
        zkclient.get_children('/clickhouse/' + cluster + '/clickhouse')
        assert_that("off", equal_to(enabled))
    except NoAuthError:
        assert_that("on", equal_to(enabled))
    except NoNodeError:
        raise RuntimeError("ClickHouse node not found in ZooKeeper")
    finally:
        zkclient.stop()
