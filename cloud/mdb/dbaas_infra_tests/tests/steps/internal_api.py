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
import requests
from behave import given, register_type, then, when
from hamcrest import (assert_that, contains_string, equal_to, has_entries, has_key, less_than, only_contains)
from parse_type import TypeBuilder

from tests.helpers import go_internal_api, internal_api
from tests.helpers.docker import run_command
from tests.helpers.internal_api import load_cluster_type_info_into_context
from tests.helpers.matchers import is_subset_of
from tests.helpers.mongodb import mongodb_alive
from tests.helpers.side_effects import side_effects
from tests.helpers.step_helpers import (get_response, get_step_data, get_timestamp, step_require)
from tests.helpers.utils import context_to_dict, merge, render_template
from tests.helpers.workarounds import retry

register_type(
    ClusterAction=TypeBuilder.make_enum({
        'modify': 'modify',
        'get': 'get',
        'remove': 'remove',
        'enable sharding in': 'enableSharding',
        'add ZooKeeper to': 'addZookeeper',
        'start failover on': 'startFailover',
        'rebalance': 'rebalance',
    }),
    ObjectAction=TypeBuilder.make_enum({
        'get': 'get',
        'add': 'add',
        'call': 'call',
        'modify': 'modify',
        'remove': 'remove',
        'grant permission to': 'grant',
        'revoke permission from': 'revoke',
    }),
    ObjectType=TypeBuilder.make_enum({
        'user': 'user',
        'users': 'user',
        'database': 'database',
        'databases': 'database',
        'host': 'host',
        'hosts': 'host',
        'shard': 'shard',
        'shards': 'shard',
        'ML model': 'mlModel',
        'ML models': 'mlModel',
        'format schema': 'formatSchema',
        'format schemas': 'formatSchema',
    }),
    ClusterType=TypeBuilder.make_enum({
        'PostgreSQL': 'postgresql',
        'ClickHouse': 'clickhouse',
        'MongoDB': 'mongodb',
        'Redis': 'redis',
        'MySQL': 'mysql',
        'ElasticSearch': 'elasticsearch',
        'OpenSearch': 'opensearch',
        'Greenplum': 'greenplum',
    }),
    QueryResultMatcher=TypeBuilder.make_enum({
        'exactly': equal_to,
        'like': is_subset_of,
        'contains': contains_string,
        'less than': less_than,
    }),
)

METHOD_MAP = {
    'get': 'GET',
    'add': 'POST',
    'modify': 'PATCH',
    'remove': 'DELETE',
}


@given('Internal API is up and running')
@given('up and running Internal API')
@retry(wait_fixed=250, stop_max_attempt_number=80)
def wait_for_internal_api(context):
    """
    Wait until DBaaS Internal API is ready to accept incoming requests.
    """
    base_url = internal_api.get_base_url(context)

    response = requests.get('{0}/ping'.format(base_url), verify=False)
    response.raise_for_status()


@when('we send request to get a list of {cluster_type:ClusterType} clusters')
@when('we send request to get a list of {cluster_type:ClusterType} clusters ' 'in folder "{folder}"')
def step_list_clusters(context, cluster_type, folder='test'):
    folder = context.conf['dynamic']['folders'].get(folder)
    context.response = internal_api.get_clusters(context, folder['folder_ext_id'], cluster_type, deserialize=False)


def get_cluster_config_by_cluster_type(context, cluster_type, config_type='standard'):
    """
    Get cluster config by cluster_type and optional config_type
    """
    available_configs = context.conf['test_cluster_configs'][cluster_type.lower()]
    return available_configs[config_type]


@given('we are working with {config_type} {cluster_type:ClusterType} cluster')
@given('we are working with {config_type} {cluster_type:ClusterType} cluster in folder "{folder_name}"')
@then('we are working with {config_type} {cluster_type:ClusterType} cluster')
@then('we are working with {config_type} {cluster_type:ClusterType} cluster in folder "{folder_name}"')
def step_define_cluster_props(context, config_type, cluster_type, folder_name='test'):
    context.cluster_config = get_cluster_config_by_cluster_type(context, cluster_type, config_type)
    context.folder = context.conf['dynamic']['folders'].get(folder_name)
    context.cloud = context.conf['dynamic']['clouds'].get(folder_name)
    context.cluster_type = cluster_type
    context.config_type = config_type
    context.client_options = {}
    load_cluster_type_info_into_context(context)
    go_internal_api.load_cluster_type_info_into_context(context, cluster_type)


@step_require('cluster')
@then('cluster environment is "{env_name}"')
def step_cluster_env(context, env_name):
    assert_that(context.cluster['environment'], equal_to(env_name))


CLUSTER_STATUS_RETRIES = 5
CLUSTER_STATUS_SLEEP = 1


@given('we are {ability:w} to log in to "{cluster_name}" ' 'with following parameters')
@then('we are {ability:w} to log in to "{cluster_name}" ' 'with following parameters')
@given('cluster "{cluster_name}" is up and running')
@then('cluster "{cluster_name}" is up and running')
@step_require('cluster_config', 'folder', 'cluster_type')
@retry(wait_fixed=1000, stop_max_attempt_number=20)  # can be unstable
def step_cluster_is_running(context, cluster_name, ability='able'):
    params = get_step_data(context)
    internal_api.load_cluster_into_context(context, cluster_name)

    def _check_pg():
        internal_api.postgres_query(
            context,
            query='SELECT 1',
            cluster_config=context.cluster_config,
            hosts=context.hosts,
            master=True,
            **params)

    def _check_ch():
        internal_api.clickhouse_query(context, query='SELECT 1', **params)

    def _check_redis():
        internal_api.redis_query(context, **params)

    def _check_mongodb():
        mongodb_alive(context, **params)

    def _check_mysql():
        internal_api.mysql_query(context, query='SELECT 1', **params)

    action_map = {
        'clickhouse': _check_ch,
        'postgresql': _check_pg,
        'redis': _check_redis,
        'mongodb': _check_mongodb,
        'mysql': _check_mysql,
    }

    try:
        action_map.get(context.cluster_type)()
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
@side_effects(
    scope='feature',
    projects=[
        'fake_solomon',
    ],
)
@step_require('folder', 'cluster_type')
def step_interact_with_cluster(context, action, cluster_name=None):
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
        context, method=method, handle=handle, data=json.dumps(context.cluster_params))

    context.operation_id = context.response.json().get('id')


def find_host(context, host_type=None, geo=None, shard_name=None):
    """
    Find first host in cluster by geo and type
    """
    for host in context.hosts:
        if geo and geo != (host.get('zoneId') or host.get('zone_id')):
            continue

        if host_type and host_type != host['type']:
            continue

        if shard_name and shard_name != (host.get('shardName') or host.get('shard_name')):
            continue

        return host['name']

    assert False, 'Unable to find required host (geo: {0}, type: {1}) in {2}'.format(
        geo or 'any', host_type or 'any', context.hosts)


@when('we attempt to add host in "{geo}" to "{cluster_name}" replication_source "{replication_source}"')
def add_cascade_replica(context, geo, cluster_name, replication_source):
    """
    Add cascade replica
    """
    internal_api.load_cluster_into_context(context, cluster_name)

    selected = find_host(context, geo=replication_source)
    context.execute_steps('''
        When we attempt to add host in cluster
        """
        hostSpecs:
          - zoneId: {geo}
            replicationSource: {replication_source}
        """
        '''.format(geo=geo, replication_source=selected))


@when('we attempt to modify host in "{geo}" in cluster "{cluster_name}" replication_source reset')
def modify_cascade_replica_reset(context, geo, cluster_name):
    """
    Modify cascade replica
    """
    internal_api.load_cluster_into_context(context, cluster_name)

    host = find_host(context, geo=geo)
    context.execute_steps('''
        When we attempt to modify hosts in cluster
        """
        updateHostSpecs:
           - hostName: {fqdn}
             replicationSource: null
        """
        '''.format(fqdn=host))


@when('we attempt to modify host in "{geo}" in cluster "{cluster_name}" '
      'replication_source geo "{replication_source}"')
def modify_cascade_replica(context, geo, cluster_name, replication_source):
    """
    Modify cascade replica
    """
    internal_api.load_cluster_into_context(context, cluster_name)

    host = find_host(context, geo=geo)
    host_repl_source = find_host(context, geo=replication_source)
    context.execute_steps('''
        When we attempt to modify hosts in cluster
        """
        updateHostSpecs:
           - hostName: {fqdn}
             replicationSource: {replication_source}
        """
        '''.format(fqdn=host, replication_source=host_repl_source))


@when('we attempt to modify host in "{geo}" in cluster "{cluster_name}" with backup_priority "{backup_priority}"')
@step_require('cluster_type')
def modify_host_backup_priority(context, geo, cluster_name, backup_priority):
    """
    Modify host backup_priority
    """
    internal_api.load_cluster_into_context(context, cluster_name)

    host = find_host(context, geo=geo)
    context.execute_steps('''
        When we attempt to modify hosts in cluster
        """
        updateHostSpecs:
           - hostName: {fqdn}
             backupPriority: {backup_priority}
        """
        '''.format(fqdn=host, backup_priority=backup_priority))


@when('we attempt to modify host in "{geo}" in cluster "{cluster_name}" with priority "{priority}"')
@step_require('cluster_type')
def modify_host_priority(context, geo, cluster_name, priority):
    """
    Modify host priority
    """
    internal_api.load_cluster_into_context(context, cluster_name)
    key = 'replicaPriority' if context.cluster_type == 'redis' else 'priority'

    host = find_host(context, geo=geo)
    context.execute_steps('''
        When we attempt to modify hosts in cluster
        """
        updateHostSpecs:
           - hostName: {fqdn}
             {key}: {priority}
        """
        '''.format(fqdn=host, key=key, priority=priority))


@when('we attempt to modify host in "{geo}" in cluster "{cluster_name}" '
      'with parameter "{parameter}" and value "{value}"')
def modify_host_parameter(context, geo, cluster_name, parameter, value):
    """
    Modify postgresql host parameter
    """
    internal_api.load_cluster_into_context(context, cluster_name)

    host = find_host(context, geo=geo)
    context.execute_steps('''
        When we attempt to modify hosts in cluster
        """
        updateHostSpecs:
           - hostName: {fqdn}
             configSpec:
               postgresqlConfig_14:
                 {parameter}: {value}
        """
        '''.format(fqdn=host, parameter=parameter, value=value))


@then('host in "{geo}" in cluster "{cluster_name}"' ' has priority "{priority}" in zookeeper')
@retry(wait_fixed=250, stop_max_attempt_number=240)
def host_has_priority(context, geo, priority, cluster_name):
    """
    Check that host has priority in zookeeper
    """
    internal_api.load_cluster_into_context(context, cluster_name)
    host = find_host(context, geo=geo)
    context.execute_steps('''
        Then zookeeper has value "{priority}" for key "{key}"
    '''.format(
        priority=priority, key='/pgsync/{cid}/all_hosts/{fqdn}/prio'.format(cid=context.cluster['id'], fqdn=host)))


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

    context.execute_steps('''
        When we attempt to remove hosts in cluster
        """
        hostNames:
          - {0}
        """
        '''.format(host))


@when('we attempt to {action:ObjectAction} {object_type:ObjectType} "{object_name}" in "{cluster_name:w}"')
@when('we attempt to {action:ObjectAction} {object_type:ObjectType} "{object_name}" in cluster')
@when('we attempt to {action:ObjectAction} {object_type:ObjectType} in "{cluster_name:w}"')
@when('we attempt to {action:ObjectAction} {object_type:ObjectType} in cluster')
@then('we {action:ObjectAction} {object_type:ObjectType} "{object_name}" in "{cluster_name:w}"')
@then('we {action:ObjectAction} {object_type:ObjectType} "{object_name}" in cluster')
@then('we {action:ObjectAction} {object_type:ObjectType} in "{cluster_name:w}"')
@then('we {action:ObjectAction} {object_type:ObjectType} in cluster')
@step_require('folder')
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
@side_effects(
    scope='feature',
    projects=[
        'minio',
        'fake_conductor',
        'zookeeper',
        'logsdb',
        'internal-api',
        'metadb',
        'fake_solomon',
    ],
)
@step_require('cluster_config', 'folder', 'cluster_type')
def step_create_cluster(context, cluster_name):
    cluster_config = merge(context.cluster_config, get_step_data(context))
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
@then('following SQL request on {master} in "{cluster_name}" succeeds without {ignore_results:w}')
@then('following SQL request on host in geo "{geo}" in ' '"{cluster_name}" succeeds')
@then('following SQL request on host in geo "{geo}" in ' '"{cluster_name}" succeeds within "{timeout}"')
@when('we execute SQL on {master:w} in "{cluster_name:Param}"')
@when('we execute SQL on {master:w} in "{cluster_name:Param}"' ' without {ignore_results:w}')
@when('we execute SQL on {master:w} in "{cluster_name:Param}" as {user}')
@when('we execute SQL on {master:w} in "{cluster_name:Param}" in database "{dbname}"')
@when('we execute SQL on {master:w} in "{cluster_name:Param}" in database "{dbname}"' ' without {ignore_results:w}')
@step_require('folder', 'cluster_config')
@retry(wait_fixed=1000, stop_max_attempt_number=20)
def step_exec_sql(context,
                  cluster_name,
                  master=False,
                  ignore_results=False,
                  geo=None,
                  timeout=None,
                  user=None,
                  dbname=None):
    # pylint: disable=too-many-arguments
    internal_api.load_cluster_into_context(context, cluster_name)
    action_map = {
        'postgresql':
            partial(
                internal_api.postgres_query,
                cluster_config=context.cluster_config,
                master=master,
                hosts=context.hosts,
                geo=geo,
                dbname=dbname,
                ignore_results=ignore_results,
            ),
        'mysql':
            partial(
                internal_api.mysql_query, master=master, geo=geo, user=user or 'another_test_user', dbname='testdb1'),
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


@then('host in geo "{geo:Param}" in "{cluster_name:Param}" is replica of host in geo "{master_geo:Param}"')
@then('host in geo "{geo:Param}" in "{cluster_name:Param}" '
      'is replica of host in geo "{master_geo:Param}" within "{timeout}"')
def step_mysql_check_replication_source(context, cluster_name, geo, master_geo, timeout="1s"):
    context.execute_steps('''
        Then following SQL request on host in geo "{geo}" in "{cn}" succeeds within "{timeout}"
        """
        SHOW SLAVE STATUS
        """
        '''.format(
        geo=geo,
        cn=cluster_name,
        timeout=timeout,
    ))
    master_host = find_host(context, geo=master_geo)
    assert_that(context.query_result['result'], only_contains(has_entries('Master_Host', master_host)))


@when('we execute the following query on {role:Param} host')
@when('we execute the following query on {role:Param} host {sharded:Param}')
@then('following query on {role:Param} host succeeds')
@then('following query on one host in "{geo:Param}" succeeds')
@when('we execute the following query on {role:Param} host of "{shard:Param}"')
@then('following query on one host of "{shard:Param}" succeeds')
@step_require('folder', 'hosts', 'cluster_config', 'cluster_type')
def step_exec_one_host(context, shard=None, geo=None, role=None, sharded=False):
    is_master = role == 'master'
    action_map = {
        'redis': partial(internal_api.redis_query, master=is_master, geo=geo, shard_name=shard, sharded=sharded),
        'clickhouse': partial(internal_api.clickhouse_query, all_hosts=False, shard_name=shard),
    }

    action = action_map.get(context.cluster_type)
    action(context, query=get_step_data(context))


@then('following query executed with credentials "{user:Param}" "{password:Param}" on all hosts fails with "{'
      'error_msg:Param}"')
@then('following query executed on hosts from zone "{zone_name:Param}" fails with "{error_msg:Param}"')
@step_require('folder', 'cluster', 'cluster_config')
def step_exec_all_hosts_fails(context, user=None, password=None, zone_name=None, error_msg=None):
    responses = internal_api.clickhouse_query(
        context,
        query=get_step_data(context),
        zone_name=zone_name,
        assume_positive=False,
        user=user,
        password=password)

    for response in responses:
        assert_that(
            str(response['value']),
            contains_string(error_msg),
            '{0} returned unexpected result: {1}'.format(response['host'], response['value']),
        )


@when('we execute the following query on all hosts with credentials "{user:Param}" "{password:Param}"')
@when('we execute the following query on all hosts')
@then('following query on all hosts succeeds')
@then('following query return "{result:Param}" on all hosts')
@when('we execute the following query on all hosts of "{shard:Param}"')
@then('following query on all hosts of "{shard:Param}" succeeds')
@then('following query return "{result:Param}" on all hosts of ' '"{shard:Param}"')
@when('we execute on all hosts with credentials "{user:Param}" "{password:Param}" and result is "{result:Param}"')
@step_require('folder', 'cluster', 'cluster_config')
def step_exec_all_hosts(context, result=None, shard=None, user=None, password=None):
    responses = internal_api.clickhouse_query(
        context, shard_name=shard, query=get_step_data(context), user=user, password=password)

    if result is not None:
        result = render_template(result, context_to_dict(context)).encode('utf-8').decode('unicode_escape')
        for response in responses:
            assert_that(
                str(response['value']),
                equal_to(result),
                '{0} returned unexpected result: {1}'.format(response['host'], response['value']),
            )


@then('following query {matches:QueryResultMatcher} "{expected:Param}" on all redis hosts')
@then('following query {matches:QueryResultMatcher} "{expected:Param}" on all redis hosts {sharded:Param}')
@when('we execute the following query on all redis hosts {sharded:Param}')
@step_require('hosts', 'conf')
def step_exec_all_redis_hosts(context, matches=None, expected=None, sharded=None):
    sharded = sharded == 'sharded'
    responses = internal_api.redis_query_all(context, query=get_step_data(context), sharded=sharded, expected=expected)

    if expected is not None:
        expected = render_template(expected, context_to_dict(context)).encode('utf-8').decode('unicode_escape')
        for host_data, response in responses.items():
            wrap = str
            # pylint: disable=comparison-with-callable
            if matches == less_than:
                # {'10.6.117.29:6379': 4245, '10.6.117.32:6379': 4245, '10.6.117.30:6379': 0,
                # '10.6.117.34:6379': 0, '10.6.117.31:6379': 0, '10.6.117.33:6379': 0}
                def max_values(resp):
                    return max([int(v) for v in resp.values()])

                wrap = max_values
                expected = int(expected)

            assert_that(
                wrap(response),
                matches(expected),
                '{0}: expected = {1}, actual = {2}'.format(" of ".join(host_data), expected, response),
            )


@when('we attempt to execute the following query on all hosts with credentials "{user:Param}" "{password:Param}"')
@when('we attempt to execute the following query on all hosts')
def step_attempt_exec_all_hosts(context, user=None, password=None):
    internal_api.clickhouse_query(
        context, query=get_step_data(context), check_response=False, user=user, password=password)


@then('query result is {matches:QueryResultMatcher}')
@step_require('query_result')
def step_check_query_result(context, matches):
    result = {
        'result': get_step_data(context),
    }
    assert_that(result, matches(context.query_result))


@then('query result is empty')
@step_require('query_result')
def step_check_query_result_empty(context):
    result = {
        'result': get_step_data(context),
    }
    assert_that(result, equal_to({'result': {}}))


def get_task(context):
    """
    Get current task
    """
    task_req = internal_api.request(
        context,
        handle='mdb/{type}/v1/operations/{operation_id}'.format(
            type=context.cluster_type, operation_id=context.operation_id),
    )
    task_req.raise_for_status()
    return task_req.json()


@then('generated task is finished within "{timeout}"')
@step_require('cluster_type', 'operation_id')
def step_task_finished(context, timeout):
    deadline = time.time() + humanfriendly.parse_timespan(timeout)
    finished_successfully = False
    error = "Task not finished until timeout"

    while time.time() < deadline:
        task = get_task(context)
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


@then('generated task is already finished')
@step_require('cluster_type', 'operation_id')
def step_task_already_finished(context):
    error = "Task not finished"

    task = get_task(context)
    assert_that(task, has_key('done'))

    finished_successfully = 'error' not in task
    if not finished_successfully:
        error = task['error']

    assert finished_successfully, '{operation_id} error is {error}'.format(
        operation_id=context.operation_id, error=error)


@then('generated task has description "{task_description}"')
@step_require('cluster_type', 'operation_id')
def step_check_task_type(context, task_description):
    task = get_task(context)
    assert_that(task, has_key('description'))
    assert_that(task['description'], equal_to(task_description))


@then('generated task is failed within "{timeout}" with error message')
@then('generated task is failed within "{timeout}" with error message "{error_message}"')
@step_require('cluster_type', 'operation_id')
def step_task_failed_error(context, timeout, error_message=None):
    error_message = error_message or context.text
    deadline = time.time() + humanfriendly.parse_timespan(timeout)

    while time.time() < deadline:
        task = get_task(context)
        assert_that(task, has_key('done'))

        if not task['done']:
            time.sleep(1)
            continue

        logging.info('Task is done: %r', task)
        assert 'error' in task, f'{context.operation_id} Task finished successfully'
        error = task['error']['message']
        assert error == error_message, f'Got invalid error: {error}'

        break


@then('generated task is failed within "{timeout}"')
@step_require('cluster_type', 'operation_id')
def step_task_failed(context, timeout):
    deadline = time.time() + humanfriendly.parse_timespan(timeout)
    error = "Task not failed until timeout"

    while time.time() < deadline:
        task = get_task(context)
        assert_that(task, has_key('done'))

        if not task['done']:
            time.sleep(1)
            continue

        logging.info('Task is done: %r', task)

        failed = 'error' in task
        if not failed:
            error = 'Task finished successfully'
        break

    assert failed, f'{context.operation_id} {error}'


@when('we create backup for {ctype:ClusterType} "{cluster_name:Param}"')
@step_require('folder')
def step_create_backup(context, cluster_name, ctype):
    cluster = internal_api.get_cluster(
        context,
        cluster_name,
        context.folder['folder_ext_id'],
    )
    url = 'mdb/{ctype}/v1/clusters/{cluster_id}:backup'
    context.response = internal_api.request(
        context, method='POST', handle=url.format(cluster_id=cluster['id'], ctype=ctype))
    context.operation_id = context.response.json().get('id')


@when('we get {ctype:ClusterType} backups for "{cluster_name:Param}"')
@then('we get {ctype:ClusterType} backups for "{cluster_name:Param}"')
@step_require('folder')
def step_get_v1_cluster_backups(context, cluster_name, ctype):
    internal_api.load_cluster_into_context(context, cluster_name)
    url = 'mdb/{ctype}/v1/clusters/{cluster_id}/backups'
    context.response = internal_api.request(
        context, method='GET', handle=url.format(cluster_id=context.cluster['id'], ctype=ctype))


def get_latest_backup_from_response(backups, shard_name=None):
    """
    Get latest backup (i.e. first backup from response).

    If shard_name is set, backups without this shard will be filtered out.
    """
    logging.info("All backups are %r", backups)

    if shard_name:
        backups = [
            backup for backup in backups
            if shard_name in backup.get('sourceShardNames', []) or shard_name in backup.get('source_shard_names', [])
        ]

    # we expect that backups are sorted in DESC order
    latest_backup = backups[0]
    logging.info('Latest backup is %r', latest_backup)

    return latest_backup


def get_latest_backup(context, response_key, shard_name=None):
    """
    Get latest backup from remembered responses
    """
    return get_latest_backup_from_response(
        get_response(context, response_key).json()['backups'], shard_name=shard_name)


def add_restore_args(context, ret, cluster_type, latest_backup, ts_key):
    """
    Add restore-specific arguments to the map
    """
    ret['backupId'] = latest_backup['id']
    if cluster_type in ('postgresql', 'mysql'):
        if ts_key is not None:
            ret['time'] = get_timestamp(context, ts_key).replace(tzinfo=datetime.timezone.utc).isoformat()
        else:
            ret['time'] = latest_backup['createdAt']
    elif cluster_type == 'mongodb' and ts_key is not None:
        target = get_timestamp(context, ts_key)
        # we restore to next operation (target ts is not included)
        ret.update({'recoveryTargetSpec': {'timestamp': target.time, 'inc': target.inc + 1}})
    return ret


@when('we restore {cluster_type:ClusterType} using latest "{backups_response_key:w}" and config')
@when(
    'we restore {cluster_type:ClusterType} using latest "{backups_response_key:w}" timestamp "{ts_key:w}" and config')
@when('we restore {cluster_type:ClusterType} using '
      'latest "{backups_response_key:w}" containing "{shard:w}" and config')
@when('we restore {cluster_type:ClusterType} using '
      'latest "{backups_response_key:w}" timestamp "{ts_key:w}" containing "{shard:w}" and config')
@step_require('folder')
def step_restore_cluster(context, cluster_type, backups_response_key, shard=None, ts_key=None):
    latest_backup = get_latest_backup(context, backups_response_key, shard_name=shard)

    restore_arguments = get_step_data(context)

    # rewrite config in context for later usage
    # and our `magic`
    default_cluster_config = get_cluster_config_by_cluster_type(context, cluster_type, context.config_type)
    context.cluster_config = merge(copy.deepcopy(default_cluster_config), copy.deepcopy(restore_arguments))

    add_restore_args(context, restore_arguments, cluster_type, latest_backup, ts_key)

    url = 'mdb/{}/v1/clusters:restore'
    context.response = internal_api.request(
        context, method='POST', handle=url.format(cluster_type), data=json.dumps(restore_arguments))
    context.operation_id = context.response.json().get('id')


@then('we are {ability} to find cluster "{cluster_name}"')
@step_require('folder')
def step_find_cluster_by_name(context, ability, cluster_name):
    try:
        internal_api.get_cluster(
            context,
            cluster_name,
            context.folder['folder_ext_id'],
        )
    except internal_api.InternalAPIError as err:
        assert ability == 'unable', str(err)


@when('we execute command on host in geo "{geo}"')
@when('we execute command on host in geo "{geo}" as user "{user}"')
def step_run_command_on_host(context, geo, user='root'):
    host = find_host(context, geo=geo)
    cmd = context.text.strip()
    retcode, output = run_command(host, cmd, user=user)
    context.command_retcode = retcode
    context.command_output = output.decode()


@then('command succeeds on all hosts')
@then('command succeeds on all hosts for user "{user}"')
def step_command_succeeds_on_all_host(context, user='root'):
    cmd = context.text.strip()
    for host in context.hosts:
        retcode, _ = run_command(host['name'], cmd, user=user)
        assert int(retcode) == 0, "command retcode is {}, not 0".format(context.command_retcode)


@then('command retcode should be "{retcode}"')
def step_command_redcode_should_be(context, retcode):
    assert hasattr(context, 'command_retcode'), "command should be executed before checking"
    assert int(retcode) == int(context.command_retcode), "command retcode is {}, not {}".format(
        context.command_retcode, retcode)


@then('command output should contain')
def step_command_output_should_contain(context):
    substr = context.text.strip()
    assert hasattr(context, 'command_output'), "command should be executed before checking"
    assert substr in (context.command_output), "command output doesn't contain {}:\n{}".format(
        substr, context.command_output)


@when('we attempt to failover cluster "{cluster_name}"')
@when('we attempt to failover cluster "{cluster_name}" to host in geo "{geo}"')
def step_failover_cluster(context, cluster_name, geo=""):
    internal_api.load_cluster_into_context(context, cluster_name)
    if geo:
        host = find_host(context, geo=geo)
        context.execute_steps('''
            When we attempt to start failover on cluster "{cluster_name}" with following parameters
            """
            hostName: {host}
            """
            '''.format(cluster_name=cluster_name, host=host))
    else:
        context.execute_steps('''
            When we attempt to start failover on cluster "{cluster_name}"
        '''.format(cluster_name=cluster_name))


@given('version at least "{version}"')
@when('version at least "{version}"')
@then('version at least "{version}"')
def step_check_version(context, version):
    req_ver_parts = version.split('.')
    current_version = context.cluster_config['configSpec'].get('version', '0')
    version_parts = current_version.split('.')
    assert len(req_ver_parts) <= len(version_parts), "Wrong version len: len({}) > len({})".format(
        req_ver_parts, version_parts)

    for i in range(min(len(req_ver_parts), len(version_parts))):
        if int(req_ver_parts[i]) > int(version_parts[i]):
            context.scenario.skip(reason="Version {version} required, found {current_version}".format(
                version=version, current_version=current_version))
        if int(req_ver_parts[i]) < int(version_parts[i]):
            break
