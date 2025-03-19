"""
Steps related to faked dataproc cluster without control-plane
"""
import json
import textwrap

from hamcrest import assert_that, equal_to, contains_string

from behave import given, then, when
from helpers import dataproc, utils, s3, cloud_logging


@given('dataproc cluster name "{name}"')
@given('cluster name "{name}"')
def set_cluster_name(ctx, name):
    """
    Put cluster name to ctx
    """
    ctx.state['cluster_name'] = name
    ctx.state['cluster'] = f'{ctx.id}-{name}'


@given('cluster with all services')
def set_cluster_full_services(ctx):
    """
    Set all available services on cluster
    """
    ctx.state['services'] = ['hdfs', 'yarn', 'mapreduce', 'tez', 'hive', 'spark',
                             'hbase', 'zookeeper', 'zeppelin', 'livy', 'oozie']


@given('cluster without services')
def set_cluster_empty_services(ctx):
    """
    Drop all services on cluster
    """
    ctx.state['services'] = []


@given('cluster with services: {services}')
def set_cluster_services(ctx, services):
    """
    Put services list to ctx
    """
    ctx.state['services'] = [service.strip() for service in services.split(',')]


@given('isolated network')
def set_isolated_network(ctx):
    """
    Emulate internal network without NAT
    """
    ctx.state['isolated_network'] = True


@given('NAT network')
def set_nat_network(ctx):
    """
    Do not set any firewall rules for emulating internal network
    """
    ctx.state['isolated_network'] = False


@given('forward static credentials for s3')
def set_s3_static_credentials(ctx):
    """
    Forward s3 static credentials and properties for using upstream integration
    """
    access_id, secret_key = ctx.state['s3_keys']
    set_property_to_value(ctx, 'core:fs.s3a.aws.credentials.provider',
                               'org.apache.hadoop.fs.s3a.SimpleAWSCredentialsProvider')
    set_property_to_value(ctx, 'core:fs.s3a.signing-algorithm', '')
    set_property_to_value(ctx, 'core:fs.s3a.access.key', access_id)
    set_property_to_value(ctx, 'core:fs.s3a.secret.key', secret_key)


@given('userdata {name} = {value}')
def set_userdata_to_value(ctx, name, value):
    """
    Set property for cluster
    """
    ctx.state.setdefault('userdata', {})[name] = value


@given('property {name} = {value}')
def set_property_to_value(ctx, name, value):
    """
    Set property for cluster
    """
    props = ctx.state.get('properties')
    if props is None:
        ctx.state['properties'] = {}
        props = ctx.state['properties']
    section, property_name = name.split(':', 1)
    if not props.get(section):
        props[section] = {}
    props[section][property_name] = utils.render_template(value, ctx)


@given('cluster has initialization_action {uri}')
@given('cluster has initialization_action {uri} with args {args}')
@given('cluster has initialization_action {uri} with timeout {timeout}')
@given('cluster has initialization_action {uri} with args {args} with timeout {timeout}')
def set_initialization_actions(ctx, uri, args=None, timeout=0):
    """
    Set initialization_actions for cluster
    """
    init_action = {}
    if args:
        init_action['args'] = json.loads(args)
    if timeout:
        init_action['timeout'] = int(timeout)
    uri = utils.render_template(uri, ctx)
    init_action['uri'] = uri
    all_init_actions = ctx.state.get('initialization_actions', [])
    all_init_actions.append(init_action)
    ctx.state['initialization_actions'] = all_init_actions


@given('s3 object {path}')
@given('s3 object {path} from {local_path}')
def step_s3_object(ctx, path, local_path=None):
    """
    Set code for s3 object and put it into bucket
    """
    data = None
    if local_path:
        with open(local_path, 'r') as fh:
            data = fh.read()
    else:
        data = textwrap.dedent(ctx.text)

    ctx.s3_object = utils.render_template(data, ctx)
    ctx.state['s3'].put_object(Body=ctx.s3_object, Bucket=ctx.id, Key=path)


@given('cluster topology is {masters} {datanodes} {computes}')
def set_cluster_topology(ctx, masters, datanodes, computes):
    """
    Set count of instances with roles
    """
    ctx.state['topology'] = (int(masters), int(datanodes), int(computes))


@given('cluster topology is singlenode')
def set_cluster_topology_singlenode(ctx):
    """
    Topology with single node (master)
    """
    set_cluster_topology(ctx, 1, 0, 0)


@given('cluster topology is lightweight')
def set_cluster_topology_no_datanode(ctx):
    """
    Topology with no data-nodes. S3 or local FS will be used instead of HDFS
    """
    set_cluster_topology(ctx, 1, 0, 1)
    set_userdata_to_value(ctx, 'data:s3_bucket', ctx.id)


@given('cluster topology with {n} {role}')
def set_cluster_topology_for_role(ctx, n, role):
    """
    Set count of instance for role
    """
    masters, datanodes, computes = ctx.state.get('topology', (1, 1, 1))
    n = int(n)
    if role not in ('masters', 'datanodes', 'computes'):
        raise Exception(f'Unknown role {role}')
    if role == 'masters':
        return set_cluster_topology(ctx, n, datanodes, computes)
    if role == 'datanodes':
        return set_cluster_topology(ctx, masters, n, computes)
    if role == 'computes':
        return set_cluster_topology(ctx, masters, datanodes, n)


@given('metastore cluster')
@given('hive metastore cluster')
def set_hive_metastore_cluster(ctx):
    if not ctx.state.get('cluster_name'):
        set_cluster_name(ctx, 'meta')
    set_nat_network(ctx)
    set_cluster_services(ctx, 'hive')
    set_cluster_topology_singlenode(ctx)


@given('lightweight spark cluster')
def set_lightweight_spark_cluster(ctx):
    set_nat_network(ctx)
    set_cluster_services(ctx, 'yarn, spark, livy')
    set_cluster_topology_no_datanode(ctx)


@when('cluster created')
@when('cluster created within {duration} minutes')
def cluster_create(ctx, duration=10):
    """
    Create faked dataproc cluster from ctx
    """
    topology = ctx.state.get('topology', (1, 1, 1))
    dataproc.dataproc_create(ctx,
                             ctx.state['cluster_name'],
                             services=ctx.state['services'],
                             properties=ctx.state.get('properties', {}),
                             initialization_actions=ctx.state.get('initialization_actions', []),
                             timeout=int(duration) * 60,
                             topology=topology)
    if ctx.state.get('s3_keys'):
        s3.wait_credentials_availability(ctx,
                                         ctx.state['s3_keys'],
                                         timeout=300)


@given('cluster is already created with ctx id `{ctx_id}`')
def cluster_already_created(ctx, ctx_id):
    """
    Fill ctx state for already created faked dataproc cluster
    """
    ctx.id = ctx_id
    ctx.state['cluster_precreated'] = True


@then('log has no syslog entries')
def has_no_syslog_entries(ctx):
    cluster_id = ctx.state.get('cluster')
    entries = cloud_logging.read(ctx, timeout=None, log_type='syslog', cluster_id=cluster_id)
    assert len(entries) == 0


@then('log has syslog entries')
@then('log has syslog entries within {timeout:d} seconds')
def has_syslog_entries(ctx, timeout=None):
    cluster_id = ctx.state.get('cluster')
    entries = cloud_logging.read(ctx, timeout=timeout, log_type='syslog', cluster_id=cluster_id)
    assert len(entries) > 0


@then('log has job log entries')
@then('log has job log entries within {timeout:d} seconds')
def has_joblog_entries(ctx, timeout=None):
    cluster_id = ctx.state.get('cluster')
    entries = cloud_logging.read(
        ctx,
        timeout=timeout,
        log_type='containers',
        cluster_id=cluster_id,
        yarn_log_type='stderr',
        message='Final app status: SUCCEEDED, exitCode: 0',
    )
    assert len(entries) > 0
    messages = [entry.message for entry in entries]
    assert 'Final app status: SUCCEEDED, exitCode: 0' in messages


@then('log has job output log entries')
@then('log has job output log entries within {timeout:d} seconds')
def has_job_output_log_entries(ctx, timeout=None):
    cluster_id = ctx.state.get('cluster')
    entries = cloud_logging.read(
        ctx,
        timeout=timeout,
        log_type='job_output',
        cluster_id=cluster_id,
    )
    assert len(entries) > 0
    messages = [entry.message for entry in entries]
    assert 'Final app status: SUCCEEDED, exitCode: 0' in messages


@then('service {service} on {role} is running')
def service_is_running(ctx, service, role):
    """
    Check that service is up and running
    """
    if role not in set(['datanodes', 'computenodes', 'masternodes']):
        raise AssertionError(f'Unknown role {role}')
    cluster = ctx.state['clusters'][ctx.state['cluster']]
    instances = cluster[role]
    for instance, is_running in dataproc.service_is_running(ctx, instances, service).items():
        assert is_running, f'service {service} is not running on instance {instance}'


@then('service {service} on {role} is not running')
def service_is_not_running(ctx, service, role):
    """
    Check that service is not running
    """
    if role not in set(['datanodes', 'computenodes', 'masternodes']):
        raise AssertionError(f'Unknown role {role}')
    cluster = ctx.state['clusters'][ctx.state['cluster']]
    instances = cluster[role]
    for instance, is_running in dataproc.service_is_running(ctx, instances, service).items():
        assert not is_running, f'service {service} is running on instance {instance}'


@when('execute command')
@when('execute command on {role}')
@when('execute command `{command}`')
@when('execute command on {role} `{command}`')
def step_execute_command(ctx, command=None, role='masternode'):
    """
    Execute command on node, default role is master
    """
    if not command:
        if not ctx.text:
            raise AssertionError('Not found command to execute')
        command = ctx.text
    command = utils.render_template(command, ctx)
    if role not in set(['masternode', 'datanode', 'computenode']):
        raise AssertionError(f'Unknown role {role}')
    instance = ctx.state['clusters'][ctx.state['cluster']][f'{role}s'][0]
    dataproc.execute_command(ctx, instance, command)


@then('command finishes')
@then('command finishes within {timeout} seconds')
@then('hive query finished')
def step_command_finished(ctx, timeout=60.0):
    """
    Wait executed command
    """
    dataproc.wait_command(ctx, float(timeout))
    command = ctx.state['command']
    out, err = command['stdout'].decode('utf-8'), command['stderr'].decode('utf-8')
    assert_that(command['exit_code'], equal_to(0), f'command failed: {command}, out: {out}, err: {err}')
    assert_that(command['timeout'], equal_to(False), f'command failed: {command}, out: {out}, err: {err}')


@then('command stdout contains')
def step_command_output_contains(ctx):
    """
    Executed command contains
    """
    command = ctx.state['command']

    if not ctx.text:
        raise AssertionError('step requires text for checking')
    if not command.get('stdout'):
        raise AssertionError('stdout is empty')
    text = utils.render_template(ctx.text, ctx)
    assert_that(command['stdout'].decode('utf-8'), contains_string(textwrap.dedent(text)))


@then('command finished with output')
@then('hive query finished with output')
def step_command_finished_with_output(ctx):
    step_command_finished(ctx)
    step_command_output_contains(ctx)


@then('command stderr contains')
def step_command_err_contains(ctx):
    """
    Executed command stderr contains
    """
    command = ctx.state['command']
    if not ctx.text:
        raise AssertionError('step requires text for checking')
    if not command.get('stderr'):
        raise AssertionError('stderr is empty')
    text = utils.render_template(ctx.text, ctx)
    assert_that(command['stderr'].decode('utf-8'), contains_string(textwrap.dedent(text)))


@when('execute hive query')
@when('execute hive query `{query}`')
def step_execute_hive_query(ctx, query=None):
    """
    Execute command on node, default role is master
    """
    if not query:
        if not ctx.text:
            raise AssertionError('Not found query to execute')
        query = ctx.text
    query = utils.render_template(query, ctx)
    command = f'sudo -u hive hive cli -v -e \'{query}\''
    step_execute_command(ctx, command)


@then('execute hive query')
@then('execute hive query `{query}`')
def step_execute_hive_query_and_check(ctx, query=None):
    step_execute_hive_query(ctx, query)
    step_command_finished(ctx)


@given('delete var {var}')
@given('var {var} is {value}')
def step_set_render_var(ctx, var, value=None):
    if value is None:
        del ctx.state['render'][var]
        return
    ctx.state['render'][var] = value


@given('masternode as a metastore')
def step_set_render_var_masternode(ctx):
    instance = ctx.state['clusters'][ctx.state['cluster']]['masternodes'][0]
    step_set_render_var(ctx, 'metastore', instance)


@then('cluster restarted')
@then('cluster {name} restarted')
def step_cluster_restarted(ctx, name=None):
    """
    Restart all instances of cluster by calling Stop/Start on each instances concurrently
    and force run bootstrap
    """
    if not name:
        name = ctx.state['cluster_name']
    dataproc.dataproc_restart(ctx, name)


@then('package {package} installed on {role}')
def step_package_installed_on_role(ctx, package, role):
    """
    Check that package installed on instances of role
    """
    if role not in set(['datanodes', 'computenodes', 'masternodes']):
        raise AssertionError(f'Unknown role {role}')
    cluster = ctx.state['clusters'][ctx.state['cluster']]
    instances = cluster[role]
    for instance, installed in dataproc.package_installed(ctx, instances, package).items():
        assert installed, f'pacakge {package} is not installed on instance {instance}'


@then('package {package} installed')
def step_package_installed(ctx, package):
    """
    Check that package installed on all instances
    """
    step_package_installed_on_role(ctx, package, 'masternodes')
    step_package_installed_on_role(ctx, package, 'datanodes')
    step_package_installed_on_role(ctx, package, 'computenodes')


@given('file {file_path}')
@given('file {file_path} on {role}')
def step_file_created(ctx, file_path: str, role: str = None):
    """
    Set code for livy statement
    """
    file_content = utils.render_template(textwrap.dedent(ctx.text), ctx)
    if role is None:
        role = 'masternodes'
    if role not in set(['datanodes', 'computenodes', 'masternodes']):
        raise AssertionError(f'Unknown role {role}')
    instances = ctx.state['clusters'][ctx.state['cluster']][role]
    dataproc.create_file(ctx, instances, file_path, file_content)
