"""
Go internal api specific steps
"""
import json
import logging
import time

import humanfriendly
from behave import given, register_type, then, when
from grpc_health.v1.health_pb2 import HealthCheckRequest, HealthCheckResponse
from grpc_health.v1.health_pb2_grpc import HealthStub
from hamcrest import assert_that, equal_to, greater_than, has_key, is_in
from parse_type import TypeBuilder

from tests.helpers import (elasticsearch, go_internal_api, greenplum, internal_api)
from tests.helpers.go_internal_api import (get_channel, get_metadata, init_service_and_message, msg_to_dict)
from tests.helpers.step_helpers import (get_response, get_step_data, print_request_on_fail, step_require)
from tests.helpers.utils import context_to_dict, merge, render_template
from tests.helpers.workarounds import retry
from tests.steps import internal_api as intapi_steps

register_type(
    ClusterType=TypeBuilder.make_enum({
        'ClickHouse': 'ClickHouse',
        'ElasticSearch': 'elasticsearch',
        'OpenSearch': 'opensearch',
        'MongoDB': 'mongodb',
        'Greenplum': 'Greenplum',
        'Redis': 'redis',
    }),
    ObjectAction=TypeBuilder.make_enum({
        'create': 'Create',
        'delete': 'Delete',
        'update': 'Update',
        'add hosts': 'AddHosts',
        'delete hosts': 'DeleteHosts',
        'grant permission to': 'GrantPermission',
        'backup': 'Backup',
        'add shard': 'AddShard',
        'delete shard': 'DeleteShard',
        'create external dictionary': 'CreateExternalDictionary',
        'update external dictionary': 'UpdateExternalDictionary',
        'delete external dictionary': 'DeleteExternalDictionary',
        'list': 'List',
        'get': 'Get',
        'start failover': 'StartFailover',
        'list hosts': 'ListHosts',
        'rebalance': 'Rebalance',
        'list clusterbackups': 'ListBackups',
    }),
    ObjectType=TypeBuilder.make_enum({
        'user': 'user_service',
        'format schema': 'format_schema_service',
        'ML model': 'ml_model_service',
        'database': 'database_service',
        'backups': 'backup_service',
        'backup': 'backup_service',
    }))


@given('Go Internal API is up and running')
@retry(wait_fixed=3000, stop_max_attempt_number=80)
def step_wait_for_deploy_api(context):
    """
    Wait until Go Internal API is ready to accept incoming requests.
    """
    with get_channel(context) as channel:
        response = HealthStub(channel).Check(HealthCheckRequest())
        # pylint: disable=no-member
        assert_that(response.status, equal_to(HealthCheckResponse.SERVING))


@when('we attempt to {action:ObjectAction} shard group in "{cluster_type:ClusterType}" cluster')
@when('we attempt to {action:ObjectAction} shard group in "{cluster_type:ClusterType}" cluster "{cluster_name:w}"')
def step_shard_group_grpc(context, action, cluster_type, cluster_name=None):
    if cluster_name:
        internal_api.load_cluster_into_context(context, cluster_name)
    params = get_step_data(context)
    params['clusterId'] = context.cluster['id']
    action += "ShardGroup"
    with get_channel(context) as channel:
        srv, msg = init_service_and_message(cluster_type, action, channel, params)
        method = getattr(srv, action)
        resp = method(msg, metadata=get_metadata(context))

    context.response = msg_to_dict(resp)
    context.operation_id = resp.id


@when('we attempt to {action:ObjectAction} {object_type:ObjectType} in "{cluster_type:ClusterType}" cluster '
      'by folderId [gRPC]')
def step_action_object_grpc_by_folder_id(context, action, cluster_type, object_type=None, cluster_name=None):
    params = get_step_data(context)

    if cluster_name:
        go_internal_api.load_cluster_into_context(context, cluster_type, cluster_name)

    if not object_type:
        object_type = "cluster_service"

    params['folderId'] = context.folder['folder_ext_id']
    with get_channel(context) as channel:
        srv, msg = init_service_and_message(cluster_type, action, channel, params, service=object_type)
        method = getattr(srv, action)
        resp = method(msg, metadata=get_metadata(context))

    context.response = msg_to_dict(resp)


@when('we {action:ObjectAction} latest {cluster_type:ClusterType} {object_type:ObjectType} '
      'from "{backups_response_key:w}" [gRPC]')
def step_latest_backups_grpc(context, action, cluster_type, object_type, backups_response_key, shard=None):
    backups = get_response(context, backups_response_key)['backups']
    latest_backup = intapi_steps.get_latest_backup_from_response(backups, shard_name=shard)

    args = get_step_data(context)
    args['backupId'] = latest_backup['id']
    with get_channel(context) as channel:
        srv, msg = init_service_and_message(cluster_type, action, channel, args, service=object_type)
        method = getattr(srv, action)
        resp = method(msg, metadata=get_metadata(context))

    context.response = msg_to_dict(resp)
    context.operation_id = resp.id


@when('we attempt to {action:ObjectAction} {object_type:ObjectType} in "{cluster_type:ClusterType}" cluster')
@when('we attempt to {action:ObjectAction} in "{cluster_type:ClusterType}" cluster [via gRPC]')
@when('we attempt to {action:ObjectAction} {object_type:ObjectType} in "{cluster_type:ClusterType}" cluster [gRPC]')
@when('we attempt to {action:ObjectAction} "{cluster_type:ClusterType}" cluster "{cluster_name:w}" [gRPC]')
@when('we attempt to {action:ObjectAction} in "{cluster_type:ClusterType}" cluster with data [gRPC]')
@when('we attempt to {action:ObjectAction} {object_type:ObjectType} in '
      '"{cluster_type:ClusterType}" cluster with data [gRPC]')
@when('we attempt to {action:ObjectAction} {object_type:ObjectType} in '
      '"{cluster_type:ClusterType}" cluster "{cluster_name:w}"')
@when('we attempt to {action:ObjectAction} {object_type:ObjectType} in '
      '"{cluster_type:ClusterType}" cluster "{cluster_name:w}" [gRPC]')
@when('we attempt to {action:ObjectAction} {object_type:ObjectType} in '
      '"{cluster_type:ClusterType}" cluster "{cluster_name:w}" with data [gRPC]')
def step_action_object_grpc(context, action, cluster_type, object_type=None, cluster_name=None):
    params = get_step_data(context)

    if cluster_name:
        go_internal_api.load_cluster_into_context(context, cluster_type, cluster_name)

    if not object_type:
        object_type = "cluster_service"

    params['cluster_id'] = context.cluster['id']
    with get_channel(context) as channel:
        srv, msg = init_service_and_message(cluster_type, action, channel, params, service=object_type)
        method = getattr(srv, action)
        resp = method(msg, metadata=get_metadata(context))

    context.response = msg_to_dict(resp)
    if getattr(resp, 'id', None):
        context.operation_id = resp.id


@when('we restore {cluster_type:ClusterType} using latest "{backups_response_key:w}" and config [gRPC]')
@when('we restore {cluster_type:ClusterType} using '
      'latest "{backups_response_key:w}" containing "{shard:w}" and config [gRPC]')
@step_require('folder')
def step_restore_cluster_grpc(context, cluster_type, backups_response_key, shard=None):
    backups = get_response(context, backups_response_key)['backups']
    latest_backup = intapi_steps.get_latest_backup_from_response(backups, shard_name=shard)

    args = get_step_data(context)
    args['backupId'] = latest_backup['id']
    args['folderId'] = context.folder['folder_ext_id']
    with get_channel(context) as channel:
        srv, msg = init_service_and_message(cluster_type, 'Restore', channel, args)
        resp = srv.Restore(msg, metadata=get_metadata(context))

    context.response = msg_to_dict(resp)
    context.operation_id = resp.id


@when('we restore {cluster_type:ClusterType} shards [{shard_names}] with config [gRPC]')
@step_require('folder')
def step_restore_sharded_cluster_grpc(context, cluster_type, shard_names: str):
    backups = get_response(context, 'backups')['backups']
    shard_names = shard_names.replace(' ', '').split(',')

    main_backup = intapi_steps.get_latest_backup_from_response(backups, shard_name=shard_names[0])

    additional_backups = []
    for shard_name in shard_names[1:]:
        additional_backups.append(intapi_steps.get_latest_backup_from_response(backups, shard_name=shard_name))

    args = get_step_data(context)
    args['backupId'] = main_backup['id']
    args['folderId'] = context.folder['folder_ext_id']
    args['additionalBackupIds'] = [backup['id'] for backup in additional_backups]

    with get_channel(context) as channel:
        srv, msg = init_service_and_message(cluster_type, 'Restore', channel, args)
        resp = srv.Restore(msg, metadata=get_metadata(context))

    context.response = msg_to_dict(resp)
    context.operation_id = resp.id


@when('we get {cluster_type:ClusterType} backups for "{cluster_name:Param}" [gRPC]')
@then('we get {cluster_type:ClusterType} backups for "{cluster_name:Param}" [gRPC]')
@step_require('folder')
def step_get_v1_cluster_backups_grpc(context, cluster_type, cluster_name):
    go_internal_api.load_cluster_into_context(context, cluster_type, cluster_name)
    params = {'cluster_id': context.cluster['id']}
    with get_channel(context) as channel:
        srv, msg = init_service_and_message(cluster_type, 'ListBackups', channel, params)
        resp = srv.ListBackups(msg, metadata=get_metadata(context))

    context.response = msg_to_dict(resp)


@then('new backups amount is larger than in "{response_key:w}"')
@step_require('response')
def step_check_there_are_more_backups(context, response_key):
    remembered_length = len(get_response(context, response_key)['backups'])
    assert_that(len(context.response['backups']), greater_than(remembered_length))


@then('backups count is greater than "{length:d}"')
@step_require('response')
def step_check_backup_count(context, length=0):
    assert_that(len(context.response['backups']), greater_than(length))


@then('backups count is equal to "{length:d}"')
@step_require('response')
def step_check_backup_length(context, length=0):
    assert_that(len(context.response['backups']), equal_to(length))


@when('we create "{cluster_type:ClusterType}" cluster "{cluster_name}" [grpc]')
@when('we create "{cluster_type:ClusterType}" cluster "{cluster_name}" with following config overrides [grpc]')
@step_require('cluster_config', 'folder', 'cluster_type')
def step_create_cluster_grpc(context, cluster_type, cluster_name):
    params = get_step_data(context)
    cluster_config = merge(context.cluster_config, params)
    cluster_config['name'] = cluster_name
    cluster_config['folder_id'] = context.folder['folder_ext_id']

    with get_channel(context) as channel:
        srv, msg = init_service_and_message(cluster_type, 'Create', channel, cluster_config)
        resp = srv.Create(msg, metadata=get_metadata(context))

    context.response = msg_to_dict(resp)
    context.operation_id = resp.id


@then('response status is "{status}" [grpc]')
@step_require('response')
def step_check_response_grpc(context, status):
    actual = context.response['response']['message']
    assert str(actual) == str(status), (
        'Got response with unexpected status ({actual}), expected: {value}, [body={body}]'.format(
            actual=actual, value=status, body=context.response))


@then('generated task is finished within "{timeout}" [grpc]')
@step_require('cluster_type', 'operation_id')
def step_task_finished_grpc(context, timeout):
    deadline = time.time() + humanfriendly.parse_timespan(timeout)
    finished_successfully = False
    error = "Task not finished until timeout"

    while time.time() < deadline:
        task = go_internal_api.get_task(context)
        assert_that(task, has_key('done'))

        if not task['done']:
            time.sleep(1)
            continue

        logging.info('Task is done: %r', task)

        finished_successfully = 'error' not in task
        if not finished_successfully:
            error = task['error']
        break

    assert finished_successfully, '{operation_id} error is {error}'.format(
        operation_id=context.operation_id, error=error)


@then('generated task has description "{task_description}" [grpc]')
@step_require('cluster_type', 'operation_id')
def step_check_task_type(context, task_description):
    task = go_internal_api.get_task(context)
    assert_that(task, has_key('description'))
    assert_that(task['description'], equal_to(task_description))


@then('generated task is finished with error in "{timeout}" with message')
@step_require('cluster_type', 'operation_id')
def step_task_finished_with_error_grpc(context, timeout):
    deadline = time.time() + humanfriendly.parse_timespan(timeout)
    message = get_step_data(context)

    while time.time() < deadline:
        task = go_internal_api.get_task(context)
        assert_that(task, has_key('done'))

        if not task['done']:
            time.sleep(1)
            continue

        logging.info('Task is done: %r', task)
        assert 'error' in task, '{operation_id} finished successfully'.format(operation_id=context.operation_id)
        break
    else:
        raise AssertionError('task {operation_id} not finished until timed out')

    assert task['error']['message'] == message, 'unexpected error message {}'.format(task['error']['message'])


@then('we are {ability:w} to log in to "{cluster_type:ClusterType}" cluster "{cluster_name}" ' 'using')
@given('"{cluster_type:ClusterType}" cluster "{cluster_name}" is up and running [grpc]')
@then('"{cluster_type:ClusterType}" cluster "{cluster_name}" is up and running [grpc]')
@step_require('cluster_config', 'folder', 'cluster_type')
@retry(wait_fixed=1000, stop_max_attempt_number=20)  # can be unstable
def step_cluster_is_running_grpc(context, cluster_type, cluster_name, ability='able'):
    params = get_step_data(context)
    go_internal_api.load_cluster_into_context(context, cluster_type, cluster_name)

    action_map = {
        'elasticsearch': elasticsearch.elasticsearch_alive,
        'opensearch': elasticsearch.elasticsearch_alive,
        'greenplum': greenplum.check_greenplum,
    }

    try:
        action_map.get(context.cluster_type)(context, **params)
        assert ability == 'able', 'Unexpected successful connect to cluster'
    except Exception as exc:
        assert ability == 'unable', repr(exc)


@when('we attempt to delete "{cluster_type:ClusterType}" cluster "{cluster_name}" [grpc]')
@step_require('folder', 'cluster_type')
def step_delete_cluster_grpc(context, cluster_type, cluster_name=None):
    context.cluster_params = get_step_data(context)
    if cluster_name:
        go_internal_api.load_cluster_into_context(context, cluster_type, cluster_name)

    data = {'cluster_id': context.cluster['id']}
    with get_channel(context) as channel:
        srv, msg = init_service_and_message(cluster_type, 'Delete', channel, data)
        resp = srv.Delete(msg, metadata=get_metadata(context))

    context.response = msg_to_dict(resp)
    context.operation_id = resp.id


@when(
    'we attempt to modify "{cluster_type:ClusterType}" cluster "{cluster_name}" with the following parameters [grpc]')
@step_require('folder', 'cluster_type')
def step_modify_cluster_grpc(context, cluster_type, cluster_name=None):
    params = get_step_data(context)
    if cluster_name:
        go_internal_api.load_cluster_into_context(context, cluster_type, cluster_name)

    params['cluster_id'] = context.cluster['id']
    with get_channel(context) as channel:
        srv, msg = init_service_and_message(cluster_type, 'Update', channel, params)
        resp = srv.Update(msg, metadata=get_metadata(context))

    context.response = msg_to_dict(resp)
    context.operation_id = resp.id


@then('we are {ability} to find "{cluster_type:ClusterType}" cluster "{cluster_name}" [grpc]')
@step_require('folder')
def step_find_cluster_by_name_grpc(context, ability, cluster_type, cluster_name):
    try:
        go_internal_api.get_cluster(
            context,
            cluster_type,
            cluster_name,
            context.folder['folder_ext_id'],
        )
    except go_internal_api.GoInternalAPIError as err:
        assert ability == 'unable', str(err)


@when('we attempt to resetup mongod hosts in cluster')
@when('we attempt to resetup mongod hosts in "{cluster_name:w}"')
def step_grpc_mongodb_resetup_hosts(context, cluster_name=None):
    """
    Call request hosts resetup.
    """
    if cluster_name:
        internal_api.load_cluster_into_context(context, cluster_name)

    params = get_step_data(context)
    params['cluster_id'] = context.cluster['id']
    with get_channel(context) as channel:
        srv, msg = init_service_and_message('mongodb', 'resetup_hosts', channel, params)
        resp = srv.ResetupHosts(msg, metadata=get_metadata(context))

    context.response = msg_to_dict(resp)
    context.operation_id = resp.id


# Unfortunately host names are not predictable
@when('we attempt to resetup mongod host from cluster')
@when('we attempt to resetup mongod host in geo "{geo}" in cluster')
@when('we attempt to resetup mongod host in geo "{geo}" in "{cluster_name}"')
@when('we attempt to resetup mongod host in geo "{geo}" in "{cluster_name}"')
@when('we attempt to resetup mongod host in geo "{geo}" in shard "{shard_name}"')
@when('we attempt to resetup mongod host in geo "{geo}" in shard "{shard_name}"')
def step_grpc_mongodb_resetup_some_host(context, host_type=None, geo=None, cluster_name=None, shard_name=None):
    if cluster_name:
        internal_api.load_cluster_into_context(context, cluster_name)

    host = intapi_steps.find_host(context, host_type, geo, shard_name)

    context.execute_steps('''
        When we attempt to resetup mongod hosts in cluster
        """
        host_names:
          - {0}
        """
        '''.format(host))


@when('we attempt to stepdown mongod hosts in cluster')
@when('we attempt to stepdown mongod hosts in "{cluster_name:w}"')
def step_grpc_mongodb_stepdown_hosts(context, cluster_name=None):
    """
    Call request hosts stepdown.
    """
    if cluster_name:
        internal_api.load_cluster_into_context(context, cluster_name)

    params = get_step_data(context)
    params['cluster_id'] = context.cluster['id']
    with get_channel(context) as channel:
        srv, msg = init_service_and_message('mongodb', 'stepdown_hosts', channel, params)
        resp = srv.ResetupHosts(msg, metadata=get_metadata(context))

    context.response = msg_to_dict(resp)
    context.operation_id = resp.id


# Unfortunately host names are not predictable
@when('we attempt to stepdown mongod host from cluster')
@when('we attempt to stepdown mongod host in geo "{geo}" in cluster')
@when('we attempt to stepdown mongod host in geo "{geo}" in "{cluster_name}"')
@when('we attempt to stepdown mongod host in geo "{geo}" in "{cluster_name}"')
@when('we attempt to stepdown mongod host in geo "{geo}" in shard "{shard_name}"')
@when('we attempt to stepdown mongod host in geo "{geo}" in shard "{shard_name}"')
def step_grpc_mongodb_stepdown_some_host(context, host_type=None, geo=None, cluster_name=None, shard_name=None):
    if cluster_name:
        internal_api.load_cluster_into_context(context, cluster_name)

    host = intapi_steps.find_host(context, host_type, geo, shard_name)

    context.execute_steps('''
        When we attempt to stepdown mongod hosts in cluster
        """
        host_names:
          - {0}
        """
        '''.format(host))


@when('we attempt to {action:ObjectAction} in "{cluster_type:ClusterType}" cluster "{cluster_name:w}"')
@when('we attempt to {action:ObjectAction} in "{cluster_type:ClusterType}" cluster')
def step_host_grpc(context, action, cluster_type, cluster_name=None):
    if cluster_name:
        go_internal_api.load_cluster_into_context(context, cluster_type, cluster_name)
    params = get_step_data(context)
    params['clusterId'] = context.cluster['id']
    with get_channel(context) as channel:
        srv, msg = init_service_and_message(cluster_type, action, channel, params)
        method = getattr(srv, action)
        resp = method(msg, metadata=get_metadata(context))

    context.response = msg_to_dict(resp)
    context.operation_id = resp.id


@when('we attempt to delete "{host_type:w}" host in "{geo:w}" from "{cluster_type:ClusterType}" '
      'cluster "{cluster_name:w}"')
@when('we attempt to delete "{host_type:w}" host in "{geo:w}" from "{cluster_type:ClusterType}" cluster')
@when('we attempt to delete "{host_type:w}" host in "{geo:w}" from "{cluster_type:ClusterType}" cluster')
def step_delete_host_grpc(context, host_type, cluster_type, geo=None, cluster_name=None):
    if cluster_name:
        go_internal_api.load_cluster_into_context(context, cluster_type, cluster_name)

    host = intapi_steps.find_host(context, host_type, geo)
    params = {
        'clusterId': context.cluster['id'],
        'host_names': [host],
    }
    with get_channel(context) as channel:
        srv, msg = init_service_and_message(cluster_type, 'DeleteHosts', channel, params)
        resp = srv.DeleteHosts(msg, metadata=get_metadata(context))

    context.response = msg_to_dict(resp)
    context.operation_id = resp.id


@when('we get {ctype:ClusterType} backups for "{cluster_name:Param}" [grpc]')
@then('we get {ctype:ClusterType} backups for "{cluster_name:Param}" [grpc]')
@step_require('folder')
def step_get_v1_cluster_backups(context, cluster_name, ctype):
    go_internal_api.load_cluster_into_context(context, ctype, cluster_name)

    params = {'cluster_id': context.cluster['id']}
    with get_channel(context) as channel:
        srv, msg = init_service_and_message(ctype, 'ListBackups', channel, params)
        resp = srv.ListBackups(msg, metadata=get_metadata(context))

    context.response = msg_to_dict(resp)


@when('we create backup for {cluster_type:ClusterType} "{cluster_name:w}" [gRPC]')
@step_require('folder')
def step_create_backup_grpc(context, cluster_type, cluster_name=None):
    context.cluster_params = get_step_data(context)
    if cluster_name:
        go_internal_api.load_cluster_into_context(context, cluster_type, cluster_name)

    data = {'cluster_id': context.cluster['id']}
    with get_channel(context) as channel:
        srv, msg = init_service_and_message(cluster_type, 'Backup', channel, data)
        resp = srv.Backup(msg, metadata=get_metadata(context))

    context.response = msg_to_dict(resp)
    context.operation_id = resp.id


def diff_ignore(expected, actual):
    """
    Check diff in dicts, lists and simple values with ignore instruction.
    :param expected:
    :param actual:
    :return:
    """
    assert_that(type(expected), equal_to(type(actual)), f"actual {actual} type differs from expected {expected} type")
    if isinstance(expected, list):
        for exp_item, act_item in zip(expected, actual):
            diff_ignore(exp_item, act_item)
    elif isinstance(expected, dict):
        for key, value in expected.items():
            assert_that(key, is_in(actual), f"expected key {key} not in {actual}")
            diff_ignore(value, actual[key])
    else:
        if expected != 'ignore':
            assert_that(expected, equal_to(actual), f"expected value {expected} not equal to {actual}")


@then('response body contains')
@step_require('response')
@print_request_on_fail
def step_check_response(context):
    actual_response_body = context.response
    if context.text:
        rendered_expected_content = render_template(context.text, context_to_dict(context))
        expected_in_response_body = json.loads(rendered_expected_content)
        diff_ignore(expected_in_response_body, actual_response_body)
