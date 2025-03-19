"""
ClickHouse specific steps
"""
from configparser import ConfigParser

import xmltodict
import yaml
from behave import given, then, when
from hamcrest import assert_that

from tests.helpers.docker import get_file, run_command
from tests.helpers.internal_api import clickhouse_query
from tests.helpers.matchers import is_subset_of
from tests.helpers.step_helpers import step_require


def _parse_conf(contents):
    """Parse config.

    We need it to check odbc.ini contents.
    """
    result = {}
    config = ConfigParser()
    config.optionxform = str  # preserve case
    config.read_string(contents)
    for sec in config.sections():
        result[sec] = {}
        for opt in config.options(sec):
            result[sec][opt] = config.get(sec, opt)
    return result


@given('ClickHouse client options')
def step_ch_client_options(context):
    context.client_options = yaml.load(context.text)


@then('config "{path}" on hosts contain the following parameters')
@then('config on hosts contain the following parameters')
@step_require('hosts')
def step_ch_check_config(context, path="/var/lib/clickhouse/preprocessed_configs/config.xml"):
    ch_hosts = [h for h in context.hosts if h['type'] == 'CLICKHOUSE']
    ctx_data = yaml.load(context.text or '') or {}
    if path.endswith('xml'):
        params = {
            'yandex': ctx_data,
        }
        parser = xmltodict.parse
    else:
        params = ctx_data
        parser = _parse_conf
    for host in ch_hosts:
        contents = get_file(host['name'], path)
        config = parser(contents)
        assert_that(params, is_subset_of(config))


@then('quota "{quota}" with duration "{duration}" for user "{user}" is "{value}"')
def check_quota(context, quota, duration, user, value):
    """
    Check user quota value.
    """
    hosts = [host for host in context.hosts if host['type'] == 'CLICKHOUSE']
    command = 'clickhouse-client --user _admin --query="SELECT {quota}' \
              ' FROM system.quota_limits WHERE duration={duration}' \
              ' AND quota_name IN (SELECT name FROM system.quotas WHERE has(apply_to_list, \'{user}\'))"'.format(
                  quota=quota, duration=duration, user=user)
    code, output = run_command(hosts[0]['name'], command)
    assert code == 0 and output.decode('utf-8').strip('\r\n') == value, \
        'Quota check response code is {0}, output is "{1}"'.format(code, output)


@then('user "{user}" created as "{request}"')
def check_show_create_user(context, user, request):
    """
    Check create user command.
    """
    hosts = [host for host in context.hosts if host['type'] == 'CLICKHOUSE']
    command = 'clickhouse-client --user _admin --query="SHOW CREATE USER {user}"'.format(user=user)
    code, output = run_command(hosts[0]['name'], command)
    assert code == 0 and output.decode('utf-8').strip('\r\n') == request, \
        'Create user response code is {0}, output is "{1}"'.format(code, output)


@when('we broke grantees for user "{user}"')
def broke_user_grantees(context, user):
    """
    Make none grantees for user
    """
    hosts = [host for host in context.hosts if host['type'] == 'CLICKHOUSE']
    command = 'clickhouse-client --user _admin --query="ALTER USER {user} GRANTEES NONE"'.format(user=user)
    run_command(hosts[0]['name'], command)


def stop_clickhouse(hostname):
    """
    Stops Clickhouse instance on given host.
    """
    run_command(hostname, 'service clickhouse-server stop')


def restart_clickhouse(hostname):
    """
    Restarts Clickhouse instance on given host.
    """
    run_command(hostname, 'service clickhouse-server restart')


def remove_databases_from_fs(hostname):
    """
    Removes Clickhouse databases from filesystem on given host.
    """
    code, output = run_command(hostname, "bash -c 'rm -rf /var/lib/clickhouse/*'")
    if code != 0:
        raise Exception("Removing data failed: " + output.decode())


def remove_table_from_fs(hostname, database, table):
    """
    Removes Clickhouse table from filesystem on given host.
    """
    code, output = run_command(
        hostname, 'rm -rf /var/lib/clickhouse/data/{database}/{table}'.format(database=database, table=table))
    if code != 0:
        raise Exception("Removing data failed: " + output.decode())
    code, output = run_command(
        hostname, 'rm -rf /var/lib/clickhouse/metadata/{database}/{table}.sql'.format(database=database, table=table))
    if code != 0:
        raise Exception("Removing data failed: " + output.decode())


def clean_zk_tables_metadata_for_host(context, host):
    """
    Recursively delete all ZK nodes belongs to host.
    """
    zk_host = next(iter(host for host in context.hosts if host['type'] == 'ZOOKEEPER'))
    command = "/usr/local/yandex/zk_cleanup.py clickhouse-hosts --root '/clickhouse/{}' --fqdn {}".format(
        host.get('clusterId') or host.get('cluster_id'),
        host['name'],
    )
    environment = {
        "LC_ALL": "C.UTF-8",
        "LANG": "C.UTF-8",
    }
    code, output = run_command(zk_host['name'], command, environment=environment)
    if code != 0:
        raise Exception("Removing zk metadata failed: " + output.decode())


def wait_for_clickhouse_up(hostname, retries=60):
    """
    Wait for clickhouse ready to work.
    """
    while retries > 0:
        retries -= 1
        code, output = run_command(hostname, 'clickhouse-client --query="SELECT 1"')
        if code == 0 and output.decode('utf-8').strip('\r\n') == '1':
            return
    raise Exception("Clickhouse dead on host " + hostname)


@when('we dirty remove all databases on hosts from zone "{zone_name:Param}"')
def step_dirty_remove_db(context, zone_name):
    hosts = [host for host in context.hosts if host['type'] == 'CLICKHOUSE' and host['zoneId'] == zone_name]
    for host in hosts:
        remove_databases_from_fs(host['name'])
        stop_clickhouse(host['name'])


@when('we remove ZK metadata for hosts from zone "{zone_name:Param}"')
def step_remove_host_zk_metadata(context, zone_name):
    hosts = [host for host in context.hosts if host['type'] == 'CLICKHOUSE' and host['zoneId'] == zone_name]
    for host in hosts:
        clean_zk_tables_metadata_for_host(context, host)


@when('we stop clickhouse-server on all hosts from zone "{zone_name:Param}"')
def step_stop_clickhouse(context, zone_name):
    hosts = [host for host in context.hosts if host['type'] == 'CLICKHOUSE' and host['zoneId'] == zone_name]
    for host in hosts:
        stop_clickhouse(host)


@then('hosts from zone "{zone_name:Param}" are dead')
def step_zone_hosts_dead(context, zone_name):
    try:
        clickhouse_query(context, query='SELECT 1', zone_name=zone_name)
        assert False, 'hosts from zone "{}" alive'.format(zone_name)
    except Exception:
        pass


@when('we remove table "{table}" in database "{database}" on all hosts')
def step_remove_table(context, database, table):
    hosts = [host for host in context.hosts if host['type'] == 'CLICKHOUSE']
    for host in hosts:
        command = 'clickhouse-client --user _admin --query="DROP TABLE {database}.{table} NO DELAY"'.format(
            database=database, table=table)
        code, output = run_command(host['name'], command)
        assert code == 0, 'DROP TABLE unsuccess, code is {0}, output is "{1}"'.format(code, output)


@when('we dirty remove table "{table}" in database "{database}" on all hosts')
def step_dirty_remove_table(context, database, table):
    hosts = [host for host in context.hosts if host['type'] == 'CLICKHOUSE']
    for host in hosts:
        remove_table_from_fs(host['name'], database, table)
        restart_clickhouse(host['name'])
    for host in hosts:
        wait_for_clickhouse_up(host['name'])


@when('we restart clickhouse on all hosts')
def step_restart_clickhouse_in_cluster(context):
    hosts = [host for host in context.hosts if host['type'] == 'CLICKHOUSE']
    for host in hosts:
        restart_clickhouse(host['name'])
    for host in hosts:
        wait_for_clickhouse_up(host['name'])
