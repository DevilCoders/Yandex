"""
Steps related to Internal API.
"""
# pylint: disable=too-many-lines

import copy
import datetime
import json
import logging
import time
from functools import partial

import humanfriendly
import yaml
from behave import given, register_type, then, when
from deepdiff import DeepDiff
from hamcrest import any_of, assert_that, contains, equal_to, has_entries, has_key, less_than, none, only_contains
from parse_type import TypeBuilder
from retrying import retry

from tests.helpers import internal_api
from tests.helpers.compute_driver import get_compute_api
from tests.helpers.generic_intapi import get_compute_instances_of_cluster
from tests.helpers.internal_api import (
    get_compute_instances_of_subcluster,
    get_subcluster,
    get_subcluster_by_name,
    get_subclusters,
    load_cluster_type_info_into_context,
)
from tests.helpers.matchers import is_subset_of
from tests.helpers.metadb import update_default_cluster_pillar
from tests.helpers.pillar import DATAPROC_PRIVATE_PORT, DATAPROC_SERVER_NAME
from tests.helpers.step_helpers import (
    get_response,
    get_step_data,
    print_request_on_fail,
    render_text,
    step_require,
    apply_overrides_to_cluster_config,
)
from tests.helpers.utils import merge, ssh
from tests.helpers import workarounds

register_type(
    ClusterAction=TypeBuilder.make_enum(
        {
            'modify': 'modify',
            'get': 'get',
            'remove': 'remove',
            'enable sharding in': 'enableSharding',
            'add ZooKeeper to': 'addZookeeper',
            'start failover on': 'startFailover',
            'rebalance': 'rebalance',
        }
    ),
    ObjectAction=TypeBuilder.make_enum(
        {
            'get': 'get',
            'add': 'add',
            'call': 'call',
            'modify': 'modify',
            'remove': 'remove',
            'grant permission to': 'grant',
            'revoke permission from': 'revoke',
        }
    ),
    ObjectType=TypeBuilder.make_enum(
        {
            'user': 'user',
            'users': 'user',
            'database': 'database',
            'databases': 'database',
            'host': 'host',
            'hosts': 'host',
            'shard': 'shard',
            'shards': 'shard',
        }
    ),
    ClusterType=TypeBuilder.make_enum(
        {
            'PostgreSQL': 'postgresql',
            'ClickHouse': 'clickhouse',
            'MongoDB': 'mongodb',
            'Redis': 'redis',
            'MySQL': 'mysql',
            'Hadoop': 'hadoop',
            'Kafka': 'kafka',
            'Metastore': 'metastore',
            'SQLServer': 'sqlserver',
            'Greenplum': 'greenplum',
        }
    ),
    QueryResultMatcher=TypeBuilder.make_enum(
        {
            'exactly': equal_to,
            'like': is_subset_of,
        }
    ),
)

METHOD_MAP = {
    'get': 'GET',
    'add': 'POST',
    'modify': 'PATCH',
    'remove': 'DELETE',
}

ALLOWED_DATA_METHODS = ['POST', 'PATCH']


@given('Internal API is up and running')
@given('up and running Internal API')
@retry(stop_max_delay=30000)
def wait_for_internal_api(context):
    """
    Wait until DBaaS Internal API is ready to accept incoming requests.
    """
    internal_api.request(context, handle='ping').raise_for_status()


@when('we send request to get a list of {cluster_type:ClusterType} clusters')
@when('we send request to get a list of {cluster_type:ClusterType} clusters ' 'in folder "{folder}"')
@print_request_on_fail
def step_list_clusters(context, cluster_type, folder='test'):
    folder = context.conf['dynamic']['folders'].get(folder)
    context.response = internal_api.get_clusters(context, folder['folder_ext_id'], cluster_type, deserialize=False)


@when('we GET subcluster with name "{subcluster_name}"')
@print_request_on_fail
def step_get_subcluster(context, subcluster_name):
    subcluster_id = get_subcluster_by_name(context, subcluster_name)['id']
    context.response = get_subcluster(context, subcluster_id)


@when('we GET cluster')
@when('we GET cluster with name "{cluster_name}"')
@print_request_on_fail
def get_cluster(context, cluster_name=None):
    if not cluster_name:
        cluster_name = context.cluster['name']
    context.response = internal_api.get_cluster_request(
        context, cluster_name, folder_id=context.folder['folder_ext_id']
    )


def get_cluster_config_by_cluster_type(context, cluster_type, config_type='standard'):
    """
    Get cluster config by cluster_type and optional config_type
    """
    available_configs = context.conf['test_cluster_configs'][cluster_type.lower()]
    # Append generated public key
    public_key = context.state['ssh_pki'].gen_pair('dataplane')['public']
    if cluster_type == 'hadoop':
        if public_key not in available_configs[config_type]['configSpec']['hadoop']['sshPublicKeys']:
            available_configs[config_type]['configSpec']['hadoop']['sshPublicKeys'].append(public_key)
    return available_configs[config_type]


def get_subcluster_config_by_cluster_type(context, role, config_type='standard'):
    """
    Get cluster config by cluster_type and optional config_type
    """
    return copy.deepcopy(context.conf['test_subcluster_config'][config_type][role.lower()])


@given('we are working with {config_type} {cluster_type:ClusterType} cluster')
@given('we are working with {config_type} {cluster_type:ClusterType} cluster in folder "{folder_name}"')
@then('we are working with {config_type} {cluster_type:ClusterType} cluster')
@then('we are working with {config_type} {cluster_type:ClusterType} cluster in folder "{folder_name}"')
@print_request_on_fail
def step_define_cluster_props(context, config_type, cluster_type, folder_name='test'):
    if cluster_type == 'sqlserver':
        update_default_cluster_pillar(context, cluster_type, ['data', 'running_in_dataproc_tests'], 'true')

    context.cluster_config = copy.deepcopy(get_cluster_config_by_cluster_type(context, cluster_type, config_type))
    context.folder = context.conf['dynamic']['folders'].get(folder_name)
    context.folder_id = context.folder['folder_ext_id']
    context.cloud = context.conf['dynamic']['clouds'].get(folder_name)
    context.cluster_type = cluster_type
    context.config_type = config_type
    context.client_options = {}
    load_cluster_type_info_into_context(context)


@step_require('cluster')
@then('cluster environment is "{env_name}"')
def step_cluster_env(context, env_name):
    assert_that(context.cluster['environment'], equal_to(env_name))


CLUSTER_STATUS_RETRIES = 5
CLUSTER_STATUS_SLEEP = 1


@given('cluster "{cluster_name}" exists')
@then('cluster "{cluster_name}" exists')
@step_require('cluster_config', 'folder', 'cluster_type')
@retry(stop_max_delay=30000)
@print_request_on_fail
def step_cluster_is_exists(context, cluster_name, ability='able'):
    internal_api.load_cluster_into_context(context, cluster_name)


@given('we are {ability:w} to log in to "{cluster_name}" ' 'with following parameters')
@then('we are {ability:w} to log in to "{cluster_name}" ' 'with following parameters')
@given('cluster "{cluster_name}" is up and running')
@then('cluster "{cluster_name}" is up and running')
@step_require('cluster_config', 'folder', 'cluster_type')
@print_request_on_fail
def step_cluster_is_running(context, cluster_name, ability='able'):
    internal_api.load_cluster_into_context(context, cluster_name)
    try:
        cluster = internal_api.get_cluster(context, context.cluster['name'], context.folder['folder_ext_id'])
        assert cluster['status'] == 'RUNNING', f"Cluster has unexpected status {cluster['status']}"
        assert cluster['health'] == 'ALIVE', f"Cluster has unexpected health {cluster['health']}"

        assert ability == 'able', 'Unexpected successful connect to cluster'
    except Exception as exc:
        assert ability == 'unable', repr(exc)


@then('all databases exist in cluster "{cluster_name}"')
@step_require('cluster_config', 'folder', 'cluster_type')
def step_databases_exist(context, cluster_name):
    def _check_ch(databases, **_):
        responses = internal_api.clickhouse_query(context, query='SELECT * FROM system.databases FORMAT JSON')
        for database in databases:
            db_name = database['name']
            for response in responses:
                found = any(data['name'] == db_name for data in json.loads(response['value'])['data'])
                assert found, "database '{db}' not found on '{host}'".format(db=db_name, host=response['host']['name'])

    action_map = {
        'clickhouse': _check_ch,
    }

    params = get_step_data(context)

    internal_api.load_cluster_into_context(context, cluster_name)

    action_map.get(context.cluster_type)(context.cluster_config['databaseSpecs'], **params)


@when('we attempt to {action:ClusterAction} cluster "{cluster_name}" with following parameters')
@when('we attempt to {action:ClusterAction} cluster "{cluster_name}"')
@when('we attempt to {action:ClusterAction} cluster')
@then('we {action:ClusterAction} cluster "{cluster_name}"')
@then('we {action:ClusterAction} cluster')
@step_require('folder', 'cluster_type')
@print_request_on_fail
def step_interact_with_cluster(context, action, cluster_name=None):
    if context.cluster_type == 'hadoop':
        context.subclusters = get_subclusters(context, context.cluster['id'])
        context.subclusters_id_by_name = {subcluster['name']: subcluster['id'] for subcluster in context.subclusters}
    context.cluster_params = get_step_data(context)

    if cluster_name:
        internal_api.load_cluster_into_context(context, cluster_name)

    handle = 'mdb/{type}/v1/clusters/{cluster_id}'.format(type=context.cluster_type, cluster_id=context.cluster['id'])

    if action in METHOD_MAP:
        method = METHOD_MAP[action]
    else:
        method = 'POST'
        handle = '{0}:{1}'.format(handle, action)

    context.response = internal_api.request(
        context, method=method, handle=handle, data=json.dumps(context.cluster_params)
    )

    context.operation_id = context.response.json().get('id')


def find_host(context, host_type=None, geo=None, shard_name=None):
    """
    Find first host in cluster by geo and type
    """
    for host in context.hosts:
        if geo and geo != host['zoneId']:
            continue

        if host_type and host_type != host['type']:
            continue

        if shard_name and shard_name != host['shardName']:
            continue

        return host['name']

    assert False, 'Unable to find required host (geo: {0}, type: {1}) in {2}'.format(
        geo or 'any', host_type or 'any', context.hosts
    )


@when('we attempt to add host in "{geo}" to "{cluster_name}" ' 'replication_source "{replication_source}"')
def add_cascade_replica(context, geo, cluster_name, replication_source):
    """
    Add postgresql cascade replica
    """
    internal_api.load_cluster_into_context(context, cluster_name)

    selected = find_host(context, geo=replication_source)
    context.execute_steps(
        '''
        When we attempt to add host in cluster
        """
        hostSpecs:
          - zoneId: {geo}
            replicationSource: {replication_source}
        """
        '''.format(
            geo=geo, replication_source=selected
        )
    )


@when('we attempt to modify host in "{geo}" in cluster "{cluster_name}" ' 'replication_source reset')
def modify_cascade_replica_reset(context, geo, cluster_name):
    """
    Modify postgresql cascade replica
    """
    internal_api.load_cluster_into_context(context, cluster_name)

    host = find_host(context, geo=geo)
    context.execute_steps(
        '''
        When we attempt to modify hosts in cluster
        """
        updateHostSpecs:
           - hostName: {fqdn}
             replicationSource: null
        """
        '''.format(
            fqdn=host
        )
    )


@when(
    'we attempt to modify host in "{geo}" in cluster "{cluster_name}" ' 'replication_source geo "{replication_source}"'
)
def modify_cascade_replica(context, geo, cluster_name, replication_source):
    """
    Modify postgresql cascade replica
    """
    internal_api.load_cluster_into_context(context, cluster_name)

    host = find_host(context, geo=geo)
    host_repl_source = find_host(context, geo=replication_source)
    context.execute_steps(
        '''
        When we attempt to modify hosts in cluster
        """
        updateHostSpecs:
           - hostName: {fqdn}
             replicationSource: {replication_source}
        """
        '''.format(
            fqdn=host, replication_source=host_repl_source
        )
    )


@when('we attempt to modify host in "{geo}" in cluster "{cluster_name}" ' 'with priority "{priority}"')
def modify_host_priority(context, geo, cluster_name, priority):
    """
    Modify postgresql host priority
    """
    internal_api.load_cluster_into_context(context, cluster_name)

    host = find_host(context, geo=geo)
    context.execute_steps(
        '''
        When we attempt to modify hosts in cluster
        """
        updateHostSpecs:
           - hostName: {fqdn}
             priority: {priority}
        """
        '''.format(
            fqdn=host, priority=priority
        )
    )


@when(
    'we attempt to modify host in "{geo}" in cluster "{cluster_name}" '
    'with parameter "{parameter}" and value "{value}"'
)
def modify_host_parameter(context, geo, cluster_name, parameter, value):
    """
    Modify postgresql host parameter
    """
    internal_api.load_cluster_into_context(context, cluster_name)

    host = find_host(context, geo=geo)
    context.execute_steps(
        '''
        When we attempt to modify hosts in cluster
        """
        updateHostSpecs:
           - hostName: {fqdn}
             configSpec:
               postgresqlConfig_10:
                 {parameter}: {value}
        """
        '''.format(
            fqdn=host, parameter=parameter, value=value
        )
    )


@then('host in "{geo}" in cluster "{cluster_name}"' ' has priority "{priority}" in zookeeper')
@workarounds.retry(wait_fixed=250, stop_max_attempt_number=240)
def host_has_priority(context, geo, priority, cluster_name):
    """
    Check that host has priority in zookeeper
    """
    internal_api.load_cluster_into_context(context, cluster_name)
    host = find_host(context, geo=geo)
    context.execute_steps(
        '''
        Then zookeeper has value "{priority}" for key "{key}"
    '''.format(
            priority=priority, key='/pgsync/{cid}/all_hosts/{fqdn}/prio'.format(cid=context.cluster['id'], fqdn=host)
        )
    )


# Unfortunately host names are not predictable
@when('we attempt to remove "{host_type}" host from cluster')
@when('we attempt to remove host in geo "{geo}" from cluster')
@when('we attempt to remove "{host_type}" host in geo "{geo}" from cluster')
@when('we attempt to remove host in geo "{geo}" from "{cluster_name}"')
@when('we attempt to remove "{host_type}" host in geo "{geo}" from "{cluster_name}"')
@when('we attempt to remove "{host_type}" host in geo "{geo}" from shard "{shard_name}"')
@when('we attempt to remove host in geo "{geo}" from shard "{shard_name}"')
@step_require('folder')
def step_remove_host(context, host_type=None, geo=None, cluster_name=None, shard_name=None):
    if cluster_name:
        internal_api.load_cluster_into_context(context, cluster_name)

    host = find_host(context, host_type, geo, shard_name)

    context.execute_steps(
        '''
        When we attempt to remove hosts in cluster
        """
        hostNames:
          - {0}
        """
        '''.format(
            host
        )
    )


@when('we attempt to {action:ObjectAction} {object_type:ObjectType} "{object_name}" in "{cluster_name:w}"')
@when('we attempt to {action:ObjectAction} {object_type:ObjectType} "{object_name}" in cluster')
@when('we attempt to {action:ObjectAction} {object_type:ObjectType} in "{cluster_name:w}"')
@when('we attempt to {action:ObjectAction} {object_type:ObjectType} in cluster')
@then('we {action:ObjectAction} {object_type:ObjectType} "{object_name}" in "{cluster_name:w}"')
@then('we {action:ObjectAction} {object_type:ObjectType} "{object_name}" in cluster')
@then('we {action:ObjectAction} {object_type:ObjectType} in "{cluster_name:w}"')
@then('we {action:ObjectAction} {object_type:ObjectType} in cluster')
@step_require('folder')
@print_request_on_fail
def step_interact_with_object(context, action, object_type, object_name=None, cluster_name=None):
    if cluster_name:
        internal_api.load_cluster_into_context(context, cluster_name)

    context.object_params = get_step_data(context)

    custom_methods = {
        'host': {
            'add': ':batchCreate',
            'remove': ':batchDelete',
            'modify': ':batchUpdate',
        },
        'user': {
            'grant': ':grantPermission',
            'revoke': ':revokePermission',
        },
    }

    url = 'mdb/{0}/v1/clusters/{1}/{2}s'.format(context.cluster_type, context.cluster['id'], object_type)

    if object_name:
        url = '{0}/{1}'.format(url, object_name)

    extra_args = {}
    if context.object_params:
        extra_args['json'] = context.object_params

    method_url_suffix = custom_methods.get(object_type, {}).get(action)
    if method_url_suffix:
        url += method_url_suffix
        method = 'POST'
    else:
        method = METHOD_MAP[action]

    context.response = internal_api.request(context, method=method, handle=url, **extra_args)

    if method != 'GET':
        context.operation_id = context.response.json().get('id')


@when('we try to create cluster "{cluster_name}" with following config overrides')
@when('we try to create cluster "{cluster_name}"')
@step_require('cluster_config', 'folder', 'cluster_type')
@print_request_on_fail
def step_create_cluster(context, cluster_name):
    cluster_config = apply_overrides_to_cluster_config(context.cluster_config, get_step_data(context))
    cluster_config['name'] = cluster_name
    cluster_config['folderId'] = context.folder['folder_ext_id']

    url = 'mdb/{0}/v1/clusters'.format(context.cluster_type)

    context.response = internal_api.request(
        context,
        method='POST',
        handle=url,
        data=json.dumps(cluster_config),
    )
    context.operation_id = context.response.json().get('id')

    for _ in range(5):
        try:
            internal_api.load_cluster_into_context(context, cluster_name)
            return
        except internal_api.InternalAPIError:
            pass
        time.sleep(1)


@then('following SQL request in "{cluster_name}" succeeds')
@then('following SQL request in "{cluster_name}" succeeds ' 'within "{timeout}"')
@then('following SQL request on {master} in "{cluster_name}" succeeds')
@then('following SQL request on {master} in "{cluster_name}" succeeds within "{timeout}"')
@then('following SQL request on host in geo "{geo}" in ' '"{cluster_name}" succeeds')
@then('following SQL request on host in geo "{geo}" in ' '"{cluster_name}" succeeds within "{timeout}"')
@when('we execute SQL on {master:w} in "{cluster_name:Param}"')
@when('we execute SQL on {master:w} in "{cluster_name:Param}"' ' without {ignore_results:w}')
@when('we execute SQL on {master:w} in "{cluster_name:Param}" as {user}')
@when('we execute SQL on {master:w} in "{cluster_name:Param}" in database "{dbname}"')
@step_require('folder', 'cluster_config')
@workarounds.retry(wait_fixed=1000, stop_max_attempt_number=20)
def step_exec_sql(
    context, cluster_name, master=False, ignore_results=False, geo=None, timeout=None, user=None, dbname=None
):
    # pylint: disable=too-many-arguments
    internal_api.load_cluster_into_context(context, cluster_name)
    action_map = {
        'postgresql': partial(
            internal_api.postgres_query,
            cluster_config=context.cluster_config,
            master=master,
            hosts=context.hosts,
            geo=geo,
            dbname=dbname,
            ignore_results=ignore_results,
        ),
        'mysql': partial(
            internal_api.mysql_query, master=master, geo=geo, user=user or 'another_test_user', dbname='testdb1'
        ),
    }
    action = action_map.get(context.cluster_type)
    if timeout is not None:
        deadline = time.time() + humanfriendly.parse_timespan(timeout)
        last_error = None
        while time.time() < deadline:
            try:
                action(context, query=context.text)
            except Exception as err:
                last_error = err
                time.sleep(1)
            else:
                last_error = None
                break
        assert last_error is None, 'query {query} failed with {error}'.format(query=context.text, error=last_error)
    else:
        action(context, query=context.text)


@then('master in "{cluster_name:Param}" has "{replics_count:d}" replics')
@then('master in "{cluster_name:Param}" has "{replics_count:d}" replics within "{timeout}"')
@then('host in geo "{geo:Param}" in "{cluster_name:Param}" ' 'has "{replics_count:d}" replics')
def step_exec_sql_and_compare_query(context, cluster_name, replics_count, geo=None, timeout=None):
    on_node = 'on master'
    if geo:
        on_node = 'on host in geo "{0}"'.format(geo)
    step = '''
        Then following SQL request {on_node} in "{cluster_name}" succeeds
        """
        SELECT count(*) AS replics FROM pg_stat_replication
        """
        And query result is like
        """
        - replics: {replics_count}
        """
        '''.format(
        on_node=on_node,
        cluster_name=cluster_name,
        replics_count=replics_count,
    )

    if timeout is not None:
        deadline = time.time() + humanfriendly.parse_timespan(timeout)
        last_error = None
        while time.time() < deadline:
            try:
                context.execute_steps(step)
            except Exception as err:
                last_error = err
                time.sleep(1)
            else:
                last_error = None
                break
        assert last_error is None, last_error
    else:
        context.execute_steps(step)


@then('host in geo "{geo:Param}" in "{cluster_name:Param}" ' 'is replica of host in geo "{master_geo:Param}"')
def step_mysql_check_replication_master(context, cluster_name, geo, master_geo):
    context.execute_steps(
        '''
        Then following SQL request on host in geo "{geo}" in "{cn}" succeeds
        """
        SHOW SLAVE STATUS
        """
        '''.format(
            geo=geo,
            cn=cluster_name,
        )
    )
    master_host = find_host(context, geo=master_geo)
    assert_that(context.query_result['result'], only_contains(has_entries('Master_Host', master_host)))


@when('we execute the following query on {role:Param} host')
@then('following query on {role:Param} host succeeds')
@then('following query on one host in "{geo:Param}" succeeds')
@when('we execute the following query on {role:Param} host of "{shard:Param}"')
@then('following query on one host of "{shard:Param}" succeeds')
@step_require('folder', 'hosts', 'cluster_config', 'cluster_type')
def step_exec_one_host(context, shard=None, geo=None, role=None):
    is_master = role == 'master'
    action_map = {
        'redis': partial(internal_api.redis_query, master=is_master, geo=geo, shard_name=shard),
        'clickhouse': partial(internal_api.clickhouse_query, all_hosts=False, shard_name=shard),
    }

    action = action_map.get(context.cluster_type)
    action(context, query=get_step_data(context))


@when('we execute the following query on all hosts')
@then('following query on all hosts succeeds')
@then('following query return "{result:Param}" on all hosts')
@when('we execute the following query on all hosts of "{shard:Param}"')
@then('following query on all hosts of "{shard:Param}" succeeds')
@then('following query return "{result:Param}" on all hosts of ' '"{shard:Param}"')
@step_require('folder', 'cluster', 'cluster_config')
def step_exec_all_hosts(context, result=None, shard=None):
    responses = internal_api.clickhouse_query(context, shard_name=shard, query=get_step_data(context))

    if result is not None:
        for response in responses:
            assert_that(
                str(response['value']),
                equal_to(result),
                '{0} returned unexpected result: {1}'.format(response['host'], response['value']),
            )


@then('query result is {matches:QueryResultMatcher}')
@step_require('query_result')
def step_check_query_result(context, matches):
    result = {
        'result': get_step_data(context),
    }
    assert_that(context.query_result, matches(result))


@then('generated task is finished within "{timeout}"')
@step_require('cluster_type', 'operation_id')
def step_task_finished(context, timeout):
    deadline = time.time() + humanfriendly.parse_timespan(timeout)
    finished_successfully = False
    error = "Task not finished until timeout"

    while time.time() < deadline:
        task = internal_api.get_task(context)
        assert_that(task, has_key('done'))

        if not task['done']:
            time.sleep(1)
            continue

        logging.debug('Task is done: %r', task)

        finished_successfully = 'error' not in task
        if not finished_successfully:
            error = task['error']
        break

    assert finished_successfully, '{operation_id} error is {error}'.format(
        operation_id=context.operation_id, error=error
    )


@then('created job fails within "{timeout}"')
@step_require('cluster_type', 'operation_id')
def step_job_failed(context, timeout):
    deadline = time.time() + humanfriendly.parse_timespan(timeout)
    failed = False
    error = "Job not terminated until timeout"

    while time.time() < deadline:
        task = internal_api.get_task(context)
        assert_that(task, has_key('done'))

        if task['done']:
            logging.debug('Task is done: %r', task)
            failed = 'error' in task
            if not failed:
                error = 'Expected job to fail but it completed successfully'
            break

        time.sleep(1)

    assert failed, error


@when('we create backup for {ctype:ClusterType} "{cluster_name:Param}"')
@step_require('folder')
@print_request_on_fail
def step_create_backup(context, cluster_name, ctype):
    cluster = internal_api.get_cluster(context, cluster_name, context.folder['folder_ext_id'])
    url = 'mdb/{ctype}/v1/clusters/{cluster_id}:backup'
    context.response = internal_api.request(
        context, method='POST', handle=url.format(cluster_id=cluster['id'], ctype=ctype)
    )
    context.operation_id = context.response.json().get('id')


@when('we get {ctype:ClusterType} backups for "{cluster_name:Param}"')
@then('we get {ctype:ClusterType} backups for "{cluster_name:Param}"')
@step_require('folder')
@print_request_on_fail
def step_get_v1_cluster_backups(context, cluster_name, ctype):
    internal_api.load_cluster_into_context(context, cluster_name)
    url = 'mdb/{ctype}/v1/clusters/{cluster_id}/backups'
    context.response = internal_api.request(
        context, method='GET', handle=url.format(cluster_id=context.cluster['id'], ctype=ctype)
    )


def get_latest_backup_from_response(backups, shard_name=None):
    """
    Get latest backup (i.e. first backup from response).

    If shard_name is set, backups without this shard will be filtered out.
    """
    logging.info("All backups are %r", backups)

    if shard_name:
        backups = [backup for backup in backups if shard_name in backup['sourceShardNames']]

    # we expect that backups are sorted in DESC order
    latest_backup = backups[0]
    logging.info('Latest backup is %r', latest_backup)

    return latest_backup


def get_latest_backup(context, response_key, shard_name=None):
    """
    Get latest backup from remembered responses
    """
    backups = get_response(context, response_key).json()['backups']
    return get_latest_backup_from_response(backups, shard_name=shard_name)


def _add_restore_args(ret, cluster_type, latest_backup, lag):
    ret['backupId'] = latest_backup['id']
    if cluster_type in ('postgresql', 'mysql'):
        if lag is not None:
            ret['time'] = (
                (
                    datetime.datetime.strptime(latest_backup['createdAt'][0:-6], '%Y-%m-%dT%H:%M:%S.%f')
                    + datetime.timedelta(seconds=int(lag or 0))
                )
                .replace(tzinfo=datetime.timezone.utc)
                .isoformat()
            )
        else:
            ret['time'] = latest_backup['createdAt']
    return ret


@when('we restore {cluster_type:ClusterType} using latest "{backups_response_key:w}" and config')
@when('we restore {cluster_type:ClusterType} using latest "{backups_response_key:w}" lag {lag} and config')
@when(
    'we restore {cluster_type:ClusterType} using ' 'latest "{backups_response_key:w}" containing "{shard:w}" and config'
)
@step_require('folder')
@print_request_on_fail
def step_restore_cluster(context, cluster_type, backups_response_key, shard=None, lag=None):
    latest_backup = get_latest_backup(context, backups_response_key, shard_name=shard)

    restore_arguments = get_step_data(context)

    # rewrite config in context for later usage
    # and our `magic`
    default_cluster_config = get_cluster_config_by_cluster_type(context, cluster_type, context.config_type)
    context.cluster_config = merge(copy.deepcopy(default_cluster_config), copy.deepcopy(restore_arguments))

    _add_restore_args(restore_arguments, cluster_type, latest_backup, lag)

    url = 'mdb/{}/v1/clusters:restore'
    context.response = internal_api.request(
        context, method='POST', handle=url.format(cluster_type), data=json.dumps(restore_arguments)
    )
    context.operation_id = context.response.json().get('id')


@then('we are {ability} to find cluster "{cluster_name}"')
@step_require('folder')
def step_find_cluster_by_name(context, ability, cluster_name):
    try:
        internal_api.get_cluster(context, cluster_name, context.folder['folder_ext_id'])
    except internal_api.InternalAPIError as err:
        assert ability == 'unable', str(err)


@then('command retcode should be "{retcode}"')
def step_command_redcode_should_be(context, retcode):
    assert hasattr(context, 'command_retcode'), "command should be executed before checking"
    assert int(retcode) == int(context.command_retcode), "command retcode is {}, not {}".format(
        context.command_retcode, retcode
    )


@then('command output should contain')
def step_command_output_should_contain(context):
    substr = context.text.strip()
    assert hasattr(context, 'command_output'), "command should be executed before checking"
    assert substr in (context.command_output), "command output doesn't contain {}:\n{}".format(
        substr, context.command_output
    )


@when('we attempt to failover cluster "{cluster_name}"')
@when('we attempt to failover cluster "{cluster_name}" to host in geo "{geo}"')
def step_failover_cluster(context, cluster_name, geo=""):
    internal_api.load_cluster_into_context(context, cluster_name)
    if geo:
        host = find_host(context, geo=geo)
        context.execute_steps(
            '''
            When we attempt to start failover on cluster "{cluster_name}" with following parameters
            """
            hostName: {host}
            """
            '''.format(
                cluster_name=cluster_name, host=host
            )
        )
    else:
        context.execute_steps(
            '''
            When we attempt to start failover on cluster "{cluster_name}"
        '''.format(
                cluster_name=cluster_name
            )
        )


@when('we try to remove cluster')
@when('we try to delete cluster')
@when('we try to remove cluster with decommission timeout "{timeout:d}" seconds')
@when('we try to delete cluster with decommission timeout "{timeout:d}" seconds')
@step_require('folder', 'cluster_type', 'cluster')
@print_request_on_fail
def step_remove_cluster(context, timeout=None):
    cid = context.cluster['id']
    url = f'mdb/{context.cluster_type}/v1/clusters/{cid}'
    if timeout:
        timeout = int(timeout)
        url += f'?decommissionTimeout={timeout}'
    context.response = internal_api.request(context, method='DELETE', handle=url)
    context.operation_id = context.response.json().get('id')


@when('we try to create subcluster "{subcluster_name}" with role "{role}"')
@step_require('folder', 'cluster', 'cluster_type')
@print_request_on_fail
def step_create_subcluster(context, subcluster_name, role):
    cid = context.cluster['id']
    url = f'mdb/{context.cluster_type}/v1/clusters/{cid}/subclusters'
    subcluster_config = merge(
        get_subcluster_config_by_cluster_type(context, role=role, config_type=context.cluster_type),
        get_step_data(context),
    )
    subcluster_config['name'] = subcluster_name

    context.response = internal_api.request(
        context,
        method='POST',
        handle=url,
        json=subcluster_config,
    )
    context.operation_id = context.response.json().get('id')


@then('all {hosts_count} hosts of subcluster "{subcluster_name}" have following labels')
def step_check_subcluster_hosts_labels(context, hosts_count, subcluster_name):
    instances = get_compute_instances_of_subcluster(context, subcluster_name, hosts_count)
    expected_labels = get_step_data(context)

    for instance in instances:
        labels = instance.labels
        assert (
            labels == expected_labels
        ), f"list of labels for host '{instance.fqdn}' differs from expected: {DeepDiff(labels, expected_labels)}"


@then('all {hosts_count} hosts of subcluster "{subcluster_name}" have {cores} cores')
def step_check_subcluster_hosts_cores(context, hosts_count, subcluster_name, cores):
    instances = get_compute_instances_of_subcluster(context, subcluster_name, hosts_count)
    for instance in instances:
        try:
            assert_that(instance.resources.cores, equal_to(int(cores)))
        except AttributeError as exception:
            raise AttributeError(f'Instance = {instance}') from exception


@then('all {hosts_count} hosts of subcluster "{subcluster_name}" have {disk_type} boot disk of size {disk_size} GB')
def step_check_subcluster_hosts_disk_size(context, hosts_count, subcluster_name, disk_type, disk_size):
    instances = get_compute_instances_of_subcluster(context, subcluster_name, hosts_count)
    for instance in instances:
        disk_id = instance.boot_disk.disk_id
        disk = get_compute_api(context).get_disk(disk_id)
        assert_that(disk.size, equal_to(int(disk_size) * 2**30))
        assert_that(disk.type_id, equal_to(disk_type))


@then('all hosts of current cluster have service account {service_account_id}')
def step_check_service_account(context, service_account_id):
    service_account_id = render_text(context, service_account_id)
    instances = get_compute_instances_of_cluster(context)
    for instance in instances:
        got_service_account_id = instance.service_account_id
        assert_that(got_service_account_id, equal_to(service_account_id))


@then('user-data attribute of metadata of all hosts of current cluster contains')
def step_check_user_data(context):
    expected_user_data = get_step_data(context)
    instances = get_compute_instances_of_cluster(context, view='FULL')
    for instance in instances:
        metadata = instance.metadata
        actual_user_data = yaml.safe_load(metadata['user-data'])['data']
        assert_that(
            actual_user_data,
            has_entries(expected_user_data),
            'Unexpected diff: {diff}'.format(diff=DeepDiff(expected_user_data, actual_user_data)),
        )


@then('all hosts of current cluster reside on the same dedicated host')
@then('all {hosts_count} hosts of current cluster reside on the same dedicated host')
def step_check_dedicated_host(context, hosts_count=None):
    instances = get_compute_instances_of_cluster(context)
    if hosts_count is not None:
        assert_that(len(instances), equal_to(int(hosts_count)), "Unexpected number of hosts in cluster")
    compute_nodes = {instance.compute_node for instance in instances}
    assert_that(
        len(compute_nodes),
        equal_to(1),
        "Expected that all VMs of cluster reside on the same dedicated host,"
        f" but got following hosts: {compute_nodes}",
    )


@then('all instances of cluster have a security_group {security_group}')
@step_require('folder', 'cluster')
def step_check_instance_security_groups(context, security_group):
    security_group = render_text(context, security_group)
    instances = get_compute_instances_of_cluster(context)
    for instance in instances:
        if not hasattr(instance, 'network_interfaces'):
            # TODO: temporary fix for IG with async vm creation
            continue
        for iface in instance.network_interfaces:
            assert_that(
                iface.security_group_ids,
                contains(security_group),
                f'Expected that all instances contains security_group: {security_group}, '
                f'but instance {instance.name}/{instance.id} have {iface.security_group_ids}',
            )


@then('all instances haven\'t security_groups')
@step_require('folder', 'cluster')
def step_check_instance_security_groups_absence(context):
    instances = get_compute_instances_of_cluster(context)
    for instance in instances:
        if not hasattr(instance, 'network_interfaces'):
            # TODO: temporary fix for IG with async vm creation
            continue
        for iface in instance.network_interfaces:
            assert_that(
                iface.security_group_ids,
                any_of(equal_to([]), none()),
                f'Expected that all instances haven\'t security_groups, '
                f'but instance {instance.name}/{instance.id} have {iface.security_group_ids}',
            )


@when('we try to modify cluster with following parameters')
@step_require('folder', 'cluster', 'cluster_type')
@print_request_on_fail
def step_modify_cluster(context):
    context.cluster_params = get_step_data(context)

    cid = context.cluster['id']
    url = f'mdb/{context.cluster_type}/v1/clusters/{cid}'

    context.response = internal_api.request(
        context,
        method='PATCH',
        handle=url,
        json=get_step_data(context),
    )
    context.operation_id = context.response.json().get('id')


@when('we try to modify subcluster "{subcluster_name}" with following parameters')
@step_require('folder', 'cluster', 'cluster_type')
@print_request_on_fail
def step_modify_subcluster(context, subcluster_name):
    context.subcluster_params = get_step_data(context)

    cid = context.cluster['id']
    subcluster = get_subcluster_by_name(context, subcluster_name)
    if subcluster is None:
        raise Exception('Subcluster is not found')
    subcid = subcluster['id']
    url = f'mdb/{context.cluster_type}/v1/clusters/{cid}/subclusters/{subcid}'

    subcluster_config = merge(
        get_subcluster_config_by_cluster_type(context, role=subcluster['role'], config_type=context.cluster_type),
        get_step_data(context),
    )
    subcluster_config['name'] = subcluster_name
    context.response = internal_api.request(
        context,
        method='PATCH',
        handle=url,
        json=subcluster_config,
    )
    context.operation_id = context.response.json().get('id')


@when('we try to stop cluster')
@when('we try to stop cluster with decommission timeout "{timeout:d}" seconds')
@step_require('folder', 'cluster_type', 'cluster')
@print_request_on_fail
def step_stop_cluster(context, timeout=None):
    cid = context.cluster['id']
    url = f'mdb/{context.cluster_type}/v1/clusters/{cid}:stop'
    if timeout:
        timeout = int(timeout)
        url += f'?decommissionTimeout={timeout}'
    context.response = internal_api.request(context, method='POST', handle=url)
    context.operation_id = context.response.json().get('id')


@when('we try to remove subcluster "{subcluster_name}"')
@when('we try to remove subcluster "{subcluster_name}" with decommission timeout "{timeout:d}" seconds')
@step_require('folder', 'cluster', 'cluster_type')
@print_request_on_fail
def step_remove_subcluster(context, subcluster_name, timeout=None):
    subcluster = get_subcluster_by_name(context, subcluster_name)
    if subcluster is None:
        raise Exception('Subcluster is not found')

    cid = context.cluster['id']
    subcid = subcluster['id']
    url = f'mdb/{context.cluster_type}/v1/clusters/{cid}/subclusters/{subcid}'
    if timeout:
        timeout = int(timeout)
        url += f'?decommissionTimeout={timeout}'

    context.response = internal_api.request(
        context,
        method='DELETE',
        handle=url,
    )
    context.operation_id = context.response.json().get('id')


@when('we {method:w} "{url}" with data')
def data_request(context, method, url, data_key=None):
    """
    Perform request with data
    """
    context.template_params = context.conf['template_params']
    data = get_step_data(context)
    assert method in ALLOWED_DATA_METHODS, 'Unexpected method {method}'.format(method=method)
    context.response = internal_api.request(
        context,
        method=method,
        handle=render_text(context, url),
        json=data,
    )


@when('we run dataproc job')
@print_request_on_fail
def step_run_dataproc_job(context):
    internal_api.ensure_cluster_is_loaded_into_context(context)
    cid = context.cluster['id']
    url = 'mdb/hadoop/v1/clusters/{0}/jobs'.format(cid)

    context.response = internal_api.request(
        context,
        method='POST',
        handle=url,
        json=get_step_data(context),
    )

    assert str(context.response.status_code) == str(200), 'Failed to submit job: {message}'.format(
        message=context.response.json()['message']
    )

    context.operation_id = context.response.json().get('id')
    context.job_id = context.response.json()['metadata']['jobId']


@when('we run dataproc job and stop cluster with decommission timeout "{timeout:d}" seconds')
@print_request_on_fail
def step_run_and_stop_dataproc_job(context, timeout):
    internal_api.ensure_cluster_is_loaded_into_context(context)
    cid = context.cluster['id']
    url = 'mdb/hadoop/v1/clusters/{0}/jobs'.format(cid)
    response = internal_api.request(
        context,
        method='POST',
        handle=url,
        json=get_step_data(context),
    )
    assert str(response.status_code) == str(200), 'Failed to submit job: {message}'.format(
        message=response.json()['message']
    )
    job_id = response.json()['metadata']['jobId']
    job_dict = internal_api.get_job(context, job_id)

    # wait until YARN job is actually started
    polling_timeout = time.time() + 600
    while job_dict['status'] in ('PENDING', 'PROVISIONING'):
        time.sleep(0.5)
        job_dict = internal_api.get_job(context, job_id)
        assert_that(time.time(), less_than(polling_timeout))
    assert_that(job_dict['status'], equal_to('RUNNING'))

    step_stop_cluster(context, timeout)


@then('we wait no more than "{timeout}" until dataproc manager returns cluster health response containing')
def step_dataproc_manager_cluster_health(context, timeout):
    expected_result = get_step_data(context)
    deadline = time.time() + humanfriendly.parse_timespan(timeout)
    gateway_hostname = context.conf['compute_driver']['fqdn']
    cid = context.cluster['id']
    method = 'yandex.cloud.priv.dataproc.manager.v1.DataprocManagerService.ClusterHealth'
    cmd = 'grpcurl -emit-defaults -plaintext -d \'{{"cid": "{cid}"}}\' {host}:{port} {method}'.format(
        cid=cid, host=DATAPROC_SERVER_NAME, port=DATAPROC_PRIVATE_PORT, method=method
    )
    contains = False
    response = {}
    while time.time() < deadline and not contains:
        code, out, err = ssh(gateway_hostname, [cmd])
        assert code == 0, f"Failed to get cluster health from dataproc manager, out: {out}, err: {err}"
        response = json.loads(out)
        contains = expected_result.items() <= response.items()
    if not contains:
        assert_that(
            response,
            has_entries(expected_result),
            'Timed out waiting until health response will contain expected entries',
        )


@then('we wait no more than "{timeout}" until cluster health is {health}')
def step_wait_cluster_health(context, timeout, health):
    deadline = time.time() + humanfriendly.parse_timespan(timeout)
    equals = False
    real_health = ""
    while time.time() < deadline and not equals:
        cluster = internal_api.get_cluster(context, context.cluster['name'], context.folder['folder_ext_id'])
        real_health = cluster['health']
        equals = real_health == health
    if not equals:
        assert_that(
            real_health, equal_to(health), 'Timed out waiting until cluster health will become equal to expected'
        )


@when('we get console cluster config')
@print_request_on_fail
def get_console_cluster_config(context):
    context.response = internal_api.get_console_cluster_config(context, folder_id=context.folder['folder_ext_id'])


@when('subcluster "{subcluster_name}" instance group id is loaded into context')
def load_subcluster_id(context, subcluster_name):
    subclusters = get_subclusters(context, context.cid)
    subcluster = next(s for s in subclusters if s['name'] == subcluster_name)
    context.instance_group_id = subcluster['instanceGroupId']
