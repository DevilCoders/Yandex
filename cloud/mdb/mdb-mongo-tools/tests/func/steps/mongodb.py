"""
Steps related to mongodb.
"""

# pylint: disable=no-name-in-module

from behave import given, register_type, then, when
from hamcrest import assert_that, contains_string, equal_to, is_in
from parse_type import TypeBuilder
from tenacity import retry, stop_after_attempt, wait_fixed

from tests.func.helpers import docker, mongodb, utils

register_type(
    FsyncLockType=TypeBuilder.make_enum({
        'fsyncLock': {
            'fsync': 1,
            'lock': True,
        },
        'fsyncUnlock': {
            'fsyncUnlock': 1,
        },
    }))


@given('a mongodb hosts {nodes}')
def step_remember_mongodb_hosts(context, nodes):
    """
    Record given mongodb nodes
    """
    context.storage.mongodb_hosts = nodes.split()


@when('all mongodb pids are saved')
def step_remember_mongodb_pids(context):
    """
    Record pids of given mongodb hosts
    """
    pids = {n: docker.pidof(context, n, 'mongod') for n in context.storage.mongodb_hosts}
    context.storage.mongodb_pids = pids


@given('a working mongodb on {node_name}')
@retry(wait=wait_fixed(1), stop=stop_after_attempt(30))
def step_wait_for_mongodb_alive(context, node_name):
    """
    Wait until mongodb is ready to accept incoming requests
    """
    conn = mongodb.mongod_connect(context, node_name, auth=False)
    ping = conn['test'].command('ping')
    if ping['ok'] != 1:
        raise RuntimeError('MongoDB is not alive')


@given('mongodb is working on all mongodb hosts')
def step_wait_for_all_mongodb_alive(context):
    """
    Wait until all mongodb hosts are ready to accept incoming requests.
    """
    for node in context.storage.mongodb_hosts:
        context.execute_steps('Given a working mongodb on {node_name}'.format(node_name=node))


@given('ReplSet has test mongodb data {test_name}')
@when('we insert test mongodb data {test_name} with write concern {write_concern}')
def step_insert_test_data(context, test_name, write_concern=None):
    """
    Load test data to mongodb
    """
    conn = mongodb.mongod_primary_connect(context, context.storage.mongodb_hosts, write_concern=write_concern)
    data = mongodb.insert_test_data(conn, mark=test_name)
    context.storage.mongodb_test_data[test_name] = data


@when('we wrap ReplSet oplog')
def step_wrap_rs_oplog(context):
    """
    Load test data to mongodb
    """
    conn = mongodb.mongod_primary_connect(context, context.storage.mongodb_hosts, ignore_unreach=True)
    mongodb.wrap_oplog(conn)


@given('oplog size is {size:d} MB on all mongodb hosts')
def step_set_oplog_size(context, size):
    """
    Set oplog size (990MB is minimum)
    """
    size_bytes = size * 1024**2
    for host in context.storage.mongodb_hosts:
        conn = mongodb.mongod_connect(context, host)
        if conn['local'].command({'collStats': 'oplog.rs'})['maxSize'] != size_bytes:
            conn['admin'].command({'replSetResizeOplog': 1, 'size': float(size)}, check=True)


@when('we make {nodes} stale')
def step_stale_mongodb(context, nodes):
    """
    Stop mongodb and wrap replset oplog
    """
    nodes_list = nodes.split()
    need_wrap_oplog = False

    for node in nodes_list:
        conn = mongodb.mongod_connect(context, node)
        host_port = mongodb.get_host_port(context, node)

        if mongodb.get_rs_member(conn, host_port)['stateStr'] == 'RECOVERING':
            continue

        if conn.is_primary:
            mongodb.step_down(conn, freeze=60)
            mongodb.wait_primary_exists(conn)

        context.execute_steps('When we stop mongodb on {node}'.format(node=node))
        need_wrap_oplog = True

    if need_wrap_oplog:
        context.execute_steps('When we wrap ReplSet oplog')

    for node in nodes_list:
        context.execute_steps('When we start mongodb on {node}'.format(node=node))

    context.execute_steps('Given mongodb is working on all mongodb hosts')

    # staled node become RECOVERING after startup
    # then becomes SECONDARY after initialization, realize it's stale
    # then becomes RECOVERING
    # so we just sleep
    # TODO: read mongodb log until 'too stale' appears
    if need_wrap_oplog:
        import time
        time.sleep(20)

    for node in nodes_list:
        context.execute_steps('Then {node} role is one of RECOVERING'.format(node=node))

    conn = mongodb.mongod_primary_connect(context, context.storage.mongodb_hosts)
    replset = conn.admin.command('replSetGetStatus')
    recovering_members = [m['name'] for m in replset['members'] if m['stateStr'] == 'RECOVERING']
    expected_recovering = [mongodb.get_host_port(context, node) for node in nodes_list]

    assert_that(recovering_members, equal_to(expected_recovering),
                'Recovering hosts are not in desired state: {rs}'.format(rs=replset))


@given('{node} role is one of {roles}')
@then('{node} role is one of {roles}')
@retry(wait=wait_fixed(1), stop=stop_after_attempt(30))
def step_check_mongodb_role(context, node, roles):
    """
    Ensure mongodb role
    """
    conn = mongodb.mongod_connect(context, node)
    replset = conn.admin.command('replSetGetStatus')
    for member in replset['members']:
        if node in member['name']:
            assert_that(member['stateStr'], is_in(roles.split()))
            return
    raise RuntimeError('Host {host} was not found in replset: {replset}'.format(host=node, replset=replset))


@given('{node_name} has test mongodb data {test_name}')
def step_fill_host_with_test_data(context, node_name, test_name):
    """
    Load test data to mongodb
    """
    conn = mongodb.mongod_connect(context, node_name)
    data = mongodb.insert_test_data(conn, mark=test_name)
    context.storage.mongodb_test_data[test_name] = data


@given('{new_node} added to replset')
def step_ensure_node_is_rs_member(context, new_node):
    """
    Add node replica set
    """
    conn = mongodb.mongod_primary_connect(context, context.storage.mongodb_hosts)
    rs_status = conn.admin.command('replSetGetStatus', check=True)
    host_port = mongodb.get_host_port(context, new_node)
    if host_port in [m['name'] for m in rs_status['members']]:
        return

    rs_conf = conn.local.system.replset.find_one()
    rs_conf['version'] += 1

    max_id = 0
    for rs_member in rs_conf['members']:
        if rs_member['_id'] > max_id:
            max_id = rs_member['_id']

    new_member_cfg = {'_id': max_id + 1, 'host': host_port}

    rs_conf['members'].append(new_member_cfg)
    conn.admin.command('replSetReconfig', rs_conf, check=True)
    context.execute_steps('Then {node} role is one of SECONDARY'.format(node=new_node))


@given('replset initialized on {node_name}')
@retry(wait=wait_fixed(1), stop=stop_after_attempt(30))
def step_ensure_rs_initialized(context, node_name):
    """
    Initialize replicaSet

    we can only do it through localhost exception
    """
    instance = docker.get_container(context, node_name)
    cmd = utils.strip_query("""
        mongo --host localhost --quiet --norc --port 27018
            --eval "rs.status()"
    """)

    _, rs_status_output = docker.exec_run(instance, cmd)
    if 'myState' in rs_status_output:
        return

    members = []
    member_id = 0
    for member in context.storage.mongodb_hosts:
        members.append({'_id': member_id, 'host': mongodb.get_host_port(context, member)})
        member_id += 1
    members[0]['priority'] = 10

    rs_conf = {'_id': mongodb.RS_NAME, 'members': members}
    if 'NotYetInitialized' in rs_status_output:
        cmd = utils.strip_query("""
                mongo --host localhost --quiet --norc --port 27018
                    --eval "rs.initiate({rs_conf})"
            """.format(rs_conf=rs_conf))
        docker.exec_run(instance, cmd)
    elif 'Unauthorized' in rs_status_output:
        conn = mongodb.mongod_connect(context, node_name)
        if mongodb.is_rs_initialized(conn):
            return

    raise RuntimeError('Replset was not initialized: {0}'.format(rs_status_output))


@given('auth initialized on {node_name}')
@retry(wait=wait_fixed(1), stop=stop_after_attempt(30))
def step_ensure_auth_initialized(context, node_name):
    """
    Create mongodb admin user

    we can only do it through localhost exception
    """
    creds = context.conf['projects']['mongodb']['users']['admin']
    instance = docker.get_container(context, node_name)
    cmd = utils.strip_query("""
        mongo --host localhost --quiet --norc --port 27018
            --eval "db.createUser({{
                user: '{username}',
                pwd: '{password}',
                roles: {roles}}})"
        {dbname}
    """.format(**creds))
    _, output = docker.exec_run(instance, cmd)
    if all(
            log not in output
            for log in ('not authorized on admin', 'Successfully added user', 'requires authentication')):
        raise RuntimeError('Can not initialize auth: {0}'.format(output))

    for node in context.storage.mongodb_hosts:
        conn = mongodb.mongod_connect(context, node)
        conn[creds['dbname']].list_collections()


@when('we {action} mongodb on {node_name}')
def step_ctl_mongodb(context, action, node_name):
    """
    Start/stop mongodb
    """
    instance = docker.get_container(context, node_name)
    cmd = utils.strip_query('/usr/bin/supervisorctl {action} mongodb'.format(action=action))
    current_exit_code, current_output = docker.exec_run(instance, cmd)

    assert_that(current_exit_code, equal_to(0), 'Unexpected exit code, command: {cmd}'.format(cmd=cmd))

    if context.text is not None:
        assert_that(current_output, contains_string(context.text))


@then('we got same user data at {nodes}')
def step_same_mongodb_data(context, nodes):
    """
    Check if user data is same on given nodes
    """
    user_data = []
    nodes_list = nodes.split()
    for node_name in nodes_list:
        conn = mongodb.mongod_connect(context, node_name)
        rows_data = mongodb.get_all_user_data(conn)
        user_data.append(rows_data)

    node1_data = user_data[0]
    assert_that(node1_data)

    for node_num in range(1, len(user_data)):
        node_data = user_data[node_num]
        assert_that(
            node_data, equal_to(node1_data), 'Data check failed host {node1} data != {node_num}'.format(
                node1=nodes_list[0], node_num=nodes_list[node_num]))


@then('we got same user data on all mongodb hosts')
def step_same_mongodb_data_all_hosts(context):
    """
    Check if user data is same on all mongodb hosts
    """
    context.execute_steps(
        'Then we got same user data at {nodes_list}'.format(nodes_list=' '.join(context.storage.mongodb_hosts)))


@then('{nodes} were {state}')
@then('{nodes} was {state}')
def step_mongodb_pids_check(context, nodes, state):
    """
    Ensure mongod instance was running or restarted
    """
    states = {'restarted': [], 'running': []}
    for node, old_pid in context.storage.mongodb_pids.items():
        current_pid = docker.pidof(context, node, 'mongod')
        key = 'restarted' if current_pid != old_pid else 'running'
        states[key].append(node)

    assert_that(states[state], equal_to(nodes.split()), 'Check status: {states}'.format(states=states))


@then('all mongodb hosts are synchronized')
@given('all mongodb hosts are synchronized')
def step_wait_mongodb_rs_synchronized(context):
    """
    Connect to each host and check opdate
    """
    conn = mongodb.mongod_primary_connect(context, context.storage.mongodb_hosts)
    mongodb.wait_secondaries_sync(conn)


@when('we freeze {node_name} for {num:d} seconds')
@then('we freeze {node_name} for {num:d} seconds')
def step_freeze_host(context, node_name, num):
    """
    Freeze/unfreeze mongodb node
    """
    conn = mongodb.mongod_connect(context, node_name)
    conn['admin'].command('replSetFreeze', num, check=True)


@when('we {action:FsyncLockType} {node_name}')
@then('we {action:FsyncLockType} {node_name}')
def step_fsynclock_host(context, action, node_name):
    """
    Freeze/unfreeze mongodb node
    """
    conn = mongodb.mongod_connect(context, node_name)
    conn['admin'].command(command=action, check=True)
