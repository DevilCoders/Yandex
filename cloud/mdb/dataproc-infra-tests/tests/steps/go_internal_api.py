"""
Steps related to Go Internal API.
"""
import os
import time
from collections import namedtuple
from functools import partial

import humanfriendly
from behave import given, register_type, then, when
from parse_type import TypeBuilder
from hamcrest import assert_that, equal_to, greater_than
from tests.helpers.matchers import is_subset_of
from tests.helpers import go_internal_api, sqlserver, utils, greenplum
from tests.helpers.step_helpers import get_step_data, step_require, get_response
from tests.helpers.workarounds import retry
import internal_api as intapi_steps

register_type(
    ClusterType=TypeBuilder.make_enum({'SQLServer': 'sqlserver', 'Greenplum': 'greenplum'}),
    QueryResultMatcher=TypeBuilder.make_enum(
        {
            'exactly': equal_to,
            'like': is_subset_of,
        }
    ),
)

TunnelInfo = namedtuple('TunnelInfo', ['port', 'username'])

TUNNELS_INFO = {'greenplum': TunnelInfo(5432, 'root')}

if os.environ.get('SQLSERVER_USE_TUNNEL'):
    TUNNELS_INFO['sqlserver'] = TunnelInfo(1433, 'Administrator')


@then('generated task is finished within "{timeout}" via GRPC')
@step_require('operation_id')
def step_task_finished(context, timeout):
    deadline = time.time() + humanfriendly.parse_timespan(timeout)
    finished_successfully = False
    error = "Task not finished until timeout"

    error, finished_successfully = go_internal_api.check_task_finished(context, deadline, error, finished_successfully)

    assert finished_successfully, '{operation_id} error is {error}'.format(
        operation_id=context.operation_id, error=error
    )


@given('"{cluster_type:ClusterType}" cluster "{cluster_name}" is up and running')
@then('"{cluster_type:ClusterType}" cluster "{cluster_name}" is up and running')
@step_require('cluster_config', 'folder', 'cluster_type')
@retry(wait_fixed=60, stop_max_attempt_number=1)  # can be unstable
def step_cluster_is_running_grpc(context, cluster_type, cluster_name):
    params = get_step_data(context)
    go_internal_api.load_cluster_into_context(context, cluster_type, cluster_name)

    hostnames = context.hosts
    if context.cluster_type == 'sqlserver':
        hostnames = [h['name'] for h in context.hosts]

    if cluster_type in TUNNELS_INFO:
        tunnel = TUNNELS_INFO[cluster_type]
        utils.open_tunnels(context, hostnames, tunnel.port, tunnel.username)

    action_map = {'sqlserver': partial(sqlserver.db_query, cluster_config=context.cluster_config, hosts=context.hosts)}

    action_map.get(context.cluster_type)(context, **params)


@given('following SQL request in {cluster_type:ClusterType} cluster "{cluster_name}" succeeds')
@then('following SQL request in {cluster_type:ClusterType} cluster "{cluster_name}" succeeds')
@then('following SQL request in {cluster_type:ClusterType} cluster "{cluster_name}" using "{dbname}" succeeds')
@then('following SQL request in {cluster_type:ClusterType} cluster "{cluster_name}" in geo "{geo}" succeeds')
@step_require('cluster_config', 'folder', 'cluster_type')
@retry(wait_fixed=60, stop_max_attempt_number=1)
def step_exec_sql_with_results(context, cluster_name, cluster_type, geo=None, master=False, dbname=None):
    step_exec_sql(context, cluster_name, cluster_type, ignore_results=False, geo=geo, master=master, dbname=dbname)


@then('following SQL request in {cluster_type:ClusterType} cluster "{cluster_name}" succeeds without results')
@then(
    'following SQL request in {cluster_type:ClusterType} cluster "{cluster_name}" using "{dbname}" succeeds without results'
)
@step_require('cluster_config', 'folder', 'cluster_type')
@retry(wait_fixed=60, stop_max_attempt_number=1)
def step_exec_sql_without_results(context, cluster_name, cluster_type, dbname=None):
    step_exec_sql(context, cluster_name, cluster_type, ignore_results=True, geo=None, dbname=dbname)


@then('following SQL request in {cluster_type:ClusterType} cluster "{cluster_name}" succeeds on master')
@then(
    'following SQL request in {cluster_type:ClusterType} cluster "{cluster_name}" using "{dbname}" succeeds on master'
)
@step_require('cluster_config', 'folder', 'cluster_type')
@retry(wait_fixed=60, stop_max_attempt_number=1)
def step_exec_sql_on_master(context, cluster_name, cluster_type, dbname=None):
    step_exec_sql(context, cluster_name, cluster_type, ignore_results=False, dbname=dbname, master=True)


@then('following SQL request in {cluster_type:ClusterType} cluster "{cluster_name}" succeeds on replica')
@then(
    'following SQL request in {cluster_type:ClusterType} cluster "{cluster_name}" using "{dbname}" succeeds on replica'
)
@step_require('cluster_config', 'folder', 'cluster_type')
@retry(wait_fixed=60, stop_max_attempt_number=1)
def step_exec_sql_on_replica(context, cluster_name, cluster_type, dbname=None):
    step_exec_sql(context, cluster_name, cluster_type, ignore_results=False, dbname=dbname, replica=True)


@then('following SQL request in {cluster_type:ClusterType} cluster "{cluster_name}" succeeds on master without results')
@then(
    'following SQL request in {cluster_type:ClusterType} cluster "{cluster_name}" using "{dbname}" succeeds on master without results'
)
@step_require('cluster_config', 'folder', 'cluster_type')
@retry(wait_fixed=60, stop_max_attempt_number=1)
def step_exec_sql_on_master_without_results(context, cluster_name, cluster_type, dbname=None):
    step_exec_sql(context, cluster_name, cluster_type, ignore_results=True, dbname=dbname, master=True)


@then(
    'following SQL request in {cluster_type:ClusterType} cluster "{cluster_name}" succeeds on replica without results'
)
@then(
    'following SQL request in {cluster_type:ClusterType} cluster "{cluster_name}" using "{dbname}" succeeds on replica without results'
)
@step_require('cluster_config', 'folder', 'cluster_type')
@retry(wait_fixed=60, stop_max_attempt_number=1)
def step_exec_sql_on_replica_without_results(context, cluster_name, cluster_type, dbname=None):
    step_exec_sql(context, cluster_name, cluster_type, ignore_results=True, dbname=dbname, replica=True)


def step_exec_sql(
    context,
    cluster_name,
    cluster_type,
    ignore_results,
    dbname=None,
    geo=None,
    master: bool = False,
    replica: bool = False,
):
    params = get_step_data(context)
    go_internal_api.load_cluster_into_context(context, cluster_type, cluster_name)

    hostnames = [h['name'] for h in context.hosts]

    if cluster_type in TUNNELS_INFO:
        tunnel = TUNNELS_INFO[cluster_type]
        utils.open_tunnels(context, hostnames, tunnel.port, tunnel.username)

    action_map = {
        'greenplum': partial(
            greenplum.db_query,
            cluster_config=context.cluster_config,
            hostnames=hostnames,
            ignore_results=ignore_results,
        ),
        'sqlserver': partial(
            sqlserver.db_query,
            cluster_config=context.cluster_config,
            hosts=context.hosts,
            ignore_results=ignore_results,
            geo=geo,
            dbname=dbname,
            master=master,
            replica=replica,
        ),
    }
    action_map.get(context.cluster_type)(context, params)


@given('query result is {matches:QueryResultMatcher}')
@then('query result is {matches:QueryResultMatcher}')
@step_require('query_result')
def step_check_query_result(context, matches):
    result = {
        'result': get_step_data(context),
    }
    assert_that(context.query_result, matches(result))


@when('we get {cluster_type:ClusterType} backups for "{cluster_name}" via GRPC')
@then('we get {cluster_type:ClusterType} backups for "{cluster_name}" via GRPC')
@step_require('folder')
def step_get_v1_cluster_backups_grpc(context, cluster_type, cluster_name):
    go_internal_api.load_cluster_into_context(context, cluster_type, cluster_name)
    context.response = go_internal_api.get_backups(context, cluster_type, context.cluster['id'])


@then('backups count is greater than "{length:d}"')
@step_require('response')
def step_check_backup_count(context, length=0):
    assert_that(len(context.response['backups']), greater_than(length))


@when('we restore {cluster_type:ClusterType} using latest "{backups_response_key:w}" and config via GRPC')
@when(
    'we restore {cluster_type:ClusterType} using '
    'latest "{backups_response_key:w}" containing "{shard:w}" and config via GRPC'
)
@step_require('folder')
def step_restore_cluster(context, cluster_type, backups_response_key, shard=None):
    backups = get_response(context, backups_response_key)['backups']

    latest_backup = intapi_steps.get_latest_backup_from_response(backups, shard_name=shard)

    args = get_step_data(context)
    args['backupId'] = latest_backup['id']
    args['folderId'] = context.folder['folder_ext_id']
    args['networkId'] = context.conf['test_managed']['networkId']

    resp = go_internal_api.restore_cluster(context, cluster_type, args)

    context.response = resp
    context.operation_id = resp.id
