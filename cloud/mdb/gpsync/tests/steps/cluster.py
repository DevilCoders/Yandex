#!/usr/bin/env python
# -*- coding: utf-8 -*-

import copy
import operator
import time

import psycopg2
import yaml

from cloud.mdb.gpsync.tests.steps import config
from cloud.mdb.gpsync.tests.steps import helpers
from cloud.mdb.gpsync.tests.steps import zk
from cloud.mdb.gpsync.tests.steps.database import Postgres, Greenplum
from behave import given, register_type, then, when
from parse_type import TypeBuilder


register_type(WasOrNot=TypeBuilder.make_enum({"was": True, "was not": False}))


@given('a "{cont_type}" container common config')
def step_common_config(context, cont_type):
    context.config[cont_type] = yaml.safe_load(context.text) or {}


def _set_use_slots_in_gpsync_config(config, use_slots):
    if 'config' not in config:
        config['config'] = {}
    if 'gpsync.conf' not in config['config']:
        config['config']['gpsync.conf'] = {}
    gpsync_conf = config['config']['gpsync.conf']
    if 'global' not in gpsync_conf:
        gpsync_conf['global'] = {}
    if 'commands' not in gpsync_conf:
        gpsync_conf['commands'] = {}
    if use_slots:
        gpsync_conf['global']['use_replication_slots'] = 'yes'
        gpsync_conf['commands']['generate_recovery_conf'] = '/usr/local/bin/gen_rec_conf_with_slot.sh %m %p'
    else:
        gpsync_conf['global']['use_replication_slots'] = 'no'
        gpsync_conf['commands']['generate_recovery_conf'] = '/usr/local/bin/gen_rec_conf_without_slot.sh %m %p'


class GPCluster(object):
    def __init__(self, members, docker_compose):
        assert isinstance(members, dict)
        self.members = members
        self.services = docker_compose['services']

        self.master_primary = None
        self.master_standby = None
        self.segments = {}

        # check all members and remember who are masters and segments
        for member, conf in members.items():
            self.add_master_primary(member, conf)
            self.add_master_standby(member, conf)
            self.add_segment(member, conf)
            assert member in self.services, 'missing config for "{name}" in compose'.format(name=member)

    def add_master_primary(self, member, conf):
        role = conf['role']
        if role == 'master_primary':
            assert self.master_primary is None, 'detected more than 1 master_primary {masters}'.format(
                masters=[self.master_primary, member]
            )
            self.master_primary = member

    def add_master_standby(self, member, conf):
        role = conf['role']
        if role == 'master_standby':
            assert self.master_standby is None, 'detected more than 1 master_standby {masters}'.format(
                masters=[self.master_standby, member]
            )
            self.master_standby = member

    def add_segment(self, member, conf):
        role = conf['role']
        if role == 'segment':
            self.segments[member] = conf

    def member_container_type(self, member):
        return 'greenplum' if self.members[member]['role'] == 'segment' else 'gpsync'

    def member_fqdn(self, member):
        return '{host}.{domain}'.format(
            host=self.services[member]['hostname'],
            domain=self.services[member]['domainname'],
        )

    def config(self, member):
        return self.members[member].get('config', dict())

    def get_master_primary(self):
        return self.master_primary

    def get_master_standby(self):
        return self.master_standby

    def get_segments(self):
        return self.segments


def execute_step_with_config(context, step, step_config):
    context.execute_steps('{step}\n"""\n{config}\n"""'.format(step=step, config=step_config))


@given('a following greenplum cluster')
def step_cluster(context):
    cluster = GPCluster(yaml.safe_load(context.text) or {}, context.compose)

    context.execute_steps(""" Given a "backup" container "backup1" """)

    # Find all zookeepers in compose and start it
    for name, service_config in context.compose['services'].items():
        image_type = helpers.build_config_get_path(service_config.get('build', 'unknown'))
        if not image_type.endswith('zookeeper'):
            continue
        context.execute_steps(
            """
            Given a "zookeeper" container "{name}"
        """.format(
                name=name
            )
        )

    # Start containers
    for member in cluster.members:
        execute_step_with_config(
            context,
            'Given a "{cont_type}" container "{name}" with following config'.format(
                cont_type=cluster.member_container_type(member), name=member
            ),
            yaml.dump(cluster.config(member), default_flow_style=False),
        )

    # Wait while containers starts in a separate cycle
    # after creation of all containers
    for member in cluster.members:
        context.execute_steps(
            """
            Then container "{name}" has status "running"
            Then container "{name}" health is "healthy"
        """.format(
                name=member
            )
        )


@given('a "{cont_type}" container "{name}"')
def step_container(context, name, cont_type):
    context.execute_steps(
        '''
        Given a "{cont_type}" container "{name}" with following config
        """
        """
    '''.format(
            name=name, cont_type=cont_type
        )
    )


@given('a "{cont_type}" container "{name}" with following config')
def step_container_with_config(context, name, cont_type):
    conf = yaml.safe_load(context.text) or {}
    docker_config = copy.deepcopy(context.compose['services'][name])

    # Check that image type is correct
    build = docker_config.pop('build')
    image_type = helpers.build_config_get_path(build)
    assert image_type.endswith(cont_type), (
        'invalid container type, '
        'expected "{cont_type}", docker-compose.yml has '
        'build "{build}"'.format(cont_type=cont_type, build=image_type)
    )

    # Pop keys that will be changed
    networks = docker_config.pop('networks')
    docker_config.pop('name', None)
    docker_config.pop('ports', None)

    # each container have its own image
    image = '{project}_{name}'.format(project=context.project, name=name)

    # create dict {container_port: None} for each container's
    # exposed port (docker will use next free port automatically)
    ports = {}
    for port in helpers.CONTAINER_PORTS[cont_type]:
        ports[port] = None

    # Unknown to docker-py field
    docker_config.pop('depends_on', None)

    if 'healthcheck' in docker_config:
        for field in ['interval', 'timeout', 'start_period']:
            value = helpers.time_to_nanoseconds(docker_config['healthcheck'].get(field))
            if value:
                docker_config['healthcheck'][field] = value

    # Create container
    container = helpers.DOCKER.containers.create(image, **docker_config, name=name, ports=ports)

    context.containers[name] = container

    # Connect container to network
    for netname, network in networks.items():
        context.networks[netname].connect(container, **network)

    # Process configs
    common_config = context.config.get(cont_type, {})
    filenames = set(list(common_config.keys()) + list(conf.keys()))
    for conffile in filenames:
        confobj = config.fromfile(conffile, helpers.container_get_conffile(container, conffile))
        # merge existing config with common config
        confobj.merge(common_config.get(conffile, {}))
        # merge existing config with step config
        confobj.merge(conf.get(conffile, {}))
        helpers.container_inject_config(container, conffile, confobj)

    container.start()
    container.reload()


@given('a replication slot "{slot_name}" in container "{name}"')
@helpers.retry_on_assert
def step_replication_slot(context, name, slot_name):
    container = context.containers[name]
    db = Postgres(host=helpers.container_get_host(), port=helpers.container_get_tcp_port(container, 5432))
    db.create_replication_slot(slot_name)


@then('container "{name}" has following replication slots')
@helpers.retry_on_assert
def step_container_has_replication_slots(context, name):
    container = context.containers[name]
    db = Postgres(host=helpers.container_get_host(), port=helpers.container_get_tcp_port(container, 5432))
    exp_values = sorted(yaml.safe_load(context.text) or [], key=operator.itemgetter('slot_name'))
    assert isinstance(exp_values, list), 'expected list, got {got}'.format(got=type(exp_values))

    actual_values = sorted(db.get_replication_slots(), key=operator.itemgetter('slot_name'))
    result_equal, err = helpers.are_dicts_subsets_of(exp_values, actual_values)

    assert result_equal, err


@then('container "{name}" is master')
def step_container_is_master(context, name):
    container = context.containers[name]
    db = Greenplum(host=helpers.container_get_host(), port=helpers.container_get_tcp_port(container, 5432))
    assert db.is_master(), 'container "{name}" is not master'.format(name=name)


@then('container "{name}" replication state is "{state}"')
@helpers.retry_on_assert
def step_container_replication_state(context, name, state):
    container = context.containers[name]
    db = Postgres(host=helpers.container_get_host(), port=helpers.container_get_tcp_port(container, 5432))
    actual_state = db.get_replication_state()[0]
    assert (
        actual_state == state
    ), f'container "{name}" replication state is "{actual_state}", while expected is "{state}"'


@then('container "{name}" became a master')
@helpers.retry_on_assert
def step_container_became_master(context, name):
    container = context.containers[name]
    db = Greenplum(host=helpers.container_get_host(), port=helpers.container_get_tcp_port(container, 5432))
    assert db.is_master(), 'container "{name}" is not master'.format(name=name)


def assert_container_is_replica(context, replica_name, master_name):
    replica = context.containers[replica_name]
    master = context.containers[master_name]
    try:
        replicadb = Greenplum(host=helpers.container_get_host(), port=helpers.container_get_tcp_port(replica, 5432))

        assert replicadb.is_master() is False, 'container "{name}" is master'.format(name=replica_name)

        masterdb = Greenplum(host=helpers.container_get_host(), port=helpers.container_get_tcp_port(master, 5432))
        replicas = masterdb.get_replication_stat()
    except psycopg2.Error as error:
        raise AssertionError(error.pgerror)

    ips = list(helpers.container_get_ip_address(replica))
    myfqdn = helpers.container_get_fqdn(replica)

    # Find replica by one of container ip addresses
    # and check that fqdn is same as container fqdn
    for stat_replica in replicas:
        if any(stat_replica['client_addr'] == ip for ip in ips):
            assert (
                stat_replica['client_hostname'] == myfqdn
            ), 'incorrect replica fqdn on master "{fqdn}", expected "{exp}"'.format(
                fqdn=stat_replica['client_hostname'], exp=myfqdn
            )
            break
    else:
        assert False, 'container {replica} is not replica of container "{master}"'.format(
            replica=replica_name, master=master_name
        )


@then('container "{replica_name}" is a replica of container "{master_name}"')
@helpers.retry_on_assert
def step_container_is_replica(context, replica_name, master_name):
    return assert_container_is_replica(context, replica_name, master_name)


@then('greenplum master in container "{name}" is up and reports all segments are also up')
@helpers.retry_on_assert
def step_greenplum_running(context, name):
    container = context.containers[name]
    db = Greenplum(host=helpers.container_get_host(), port=helpers.container_get_tcp_port(container, 5432))
    assert db.ping(), 'Greenplum is not running in container "{name}"'.format(name=name)
    assert db.all_segments_are_up(), 'Greenplum does not have all segments up in container "{name}"'.format(name=name)


@given('a common started gpsync-controlled greenplum cluster')
def step_greenplum_running_cluster(context):
    context.execute_steps(
        '''
        Given a following greenplum cluster
        """
          greenplum_master1:
            role: master_primary
          greenplum_master2:
            role: master_standby
          greenplum_segment1:
            role: segment
          greenplum_segment2:
            role: segment
        """
        When we start "gpsync" in container "greenplum_master1"
        And we start "gpsync" in container "greenplum_master2"
        Then greenplum master in container "greenplum_master1" is up and reports all segments are also up
        And container "greenplum_master2" is in sync group
        And pgbouncer is running in container "greenplum_master1"
        But pgbouncer is not running in container "greenplum_master2"
    '''
    )


@then('greenplum master in container "{name}" is not running')
def step_greenplum_not_running(context, name):
    context.execute_steps(
        """
        Then "greenplum" is not running in container "{name}"
        """.format(
            name=name
        )
    )


@then('greenplum master in container "{name}" is recovering')
@helpers.retry_on_assert
def step_greenplum_recovering(context, name):
    container = context.containers[name]
    db = Greenplum(host=helpers.container_get_host(), port=helpers.container_get_tcp_port(container, 5432))
    assert db.replica_in_recovery_mode, "Greenplum standby master is not in recovery mode"


@when('we remove standby master from greenplum cluster in container "{name}"')
def step_remove_standby_master(context, name):
    container = context.containers[name]
    db = Greenplum(host=helpers.container_get_host(), port=helpers.container_get_tcp_port(container, 5432))
    db.remove_standby_master()


@then('pgbouncer is running in container "{name}"')
@helpers.retry_on_assert
def step_pgbouncer_running(context, name):
    container = context.containers[name]
    db = Greenplum(host=helpers.container_get_host(), port=helpers.container_get_tcp_port(container, 6432))
    assert db.ping(), 'pgbouncer is not running in container "{name}"'.format(name=name)


@then('pgbouncer is not running in container "{name}"')
def step_pgbouncer_not_running(context, name):
    context.execute_steps(
        """
        Then "pgbouncer" is not running in container "{name}"
        """.format(
            name=name
        )
    )


@then('"{service}" is not running in container "{name}"')
@helpers.retry_on_assert
def step_service_not_running(context, service, name):
    container = context.containers[name]
    try:
        Greenplum(
            host=helpers.container_get_host(),
            port=helpers.container_get_tcp_port(container, helpers.SERVICE_PORTS[service]),
        )
    except AssertionError as ae:
        err = ae.args[0]
        if isinstance(err, psycopg2.OperationalError) and any(
            match in err.args[0]
            for match in [
                'Connection refused',  # If container is shut and docker-proxy is not listening
                'timeout expired',  # If container is disconnected from network and not reachable within timeout
                'server closed the connection unexpectedly',  # If docker-proxy accepted connection but bouncer is down
            ]
        ):
            # service is really not running, it is what we want
            return

        raise AssertionError(
            f'{service} is running in container "{name}" but connection can\'t be established. Error is {err!r}'
        )
    # pgbouncer is running
    raise AssertionError(f'{service} is running in container "{name}"')


@then('container "{name}" has following config')
@helpers.retry_on_assert
def step_container_has_config(context, name):
    container = context.containers[name]
    conf = yaml.safe_load(context.text) or {}
    for conffile, confvalue in conf.items():
        confobj = config.fromfile(conffile, helpers.container_get_conffile(container, conffile))
        valid, err = confobj.check_values_equal(confvalue)
        assert valid, err


@then('greenplum master in container "{name:w}" has value "{value}" for option "{option:w}"')
@then('greenplum master in container "{name:w}" has empty option "{option}"')
@helpers.retry_on_assert
def step_greenplum_master_option_has_value(context, name, option, value=''):
    container = context.containers[name]
    db = Greenplum(host=helpers.container_get_host(), port=helpers.container_get_tcp_port(container, 5432))
    val = db.get_config_option(option)
    assert val == value, 'option "{opt}" has value "{val}", expected "{exp}"'.format(opt=option, val=val, exp=value)


@when('run in container "{name:w}" "{sessions:d}" sessions with timeout {timeout:d}')
@helpers.retry_on_assert
def step_postgresql_make_sessions(context, name, sessions, timeout):
    container = context.containers[name]
    for connect in range(sessions):
        db = Postgres(
            host=helpers.container_get_host(),
            port=helpers.container_get_tcp_port(container, 5432),
            async_=True,
        )
        db.pg_sleep(timeout)


@then('pgbouncer in container "{name}" has value "{value}" for option "{option}"')
@helpers.retry_on_assert
def step_pgbouncer_option_has_value(context, name, option, value):
    container = context.containers[name]
    db = Postgres(
        dbname='pgbouncer',
        host=helpers.container_get_host(),
        port=helpers.container_get_tcp_port(container, 6432),
        autocommit=True,
    )
    db.cursor.execute('SHOW config')
    for row in db.cursor.fetchall():
        if str(row['key']) == str(option):
            assert row['value'] == value, 'option "{opt}" has value "{val}", expected "{exp}"'.format(
                opt=option, val=row['value'], exp=value
            )
            break
    else:
        assert False, 'missing option "{opt}" in pgboncer config'.format(opt=option)


@then('container "{name}" has status "{status}"')
@helpers.retry_on_assert
def step_container_status(context, name, status):
    container = context.containers[name]
    container.reload()
    current_status = helpers.container_get_status(container)
    expected_status = str(status).lower()
    assert current_status == expected_status, 'Unexpected container state "{state}", expected "{exp}"'.format(
        state=current_status, exp=status
    )


@then('container "{name}" health is "{status}"')
@helpers.retry_on_assert
def step_container_health_status(context, name, status):
    container = context.containers[name]
    container.reload()
    current_status = helpers.container_get_health_status(container)
    expected_status = str(status).lower()
    assert current_status == expected_status, 'Unexpected container health "{state}", expected "{exp}"'.format(
        state=current_status, exp=status
    )


@when('we kill container "{name}" with signal "{signal}"')
def step_kill_container(context, name, signal):
    container = context.containers[name]
    helpers.kill(container, signal)
    container.reload()


def ensure_exec(context, container_name, cmd):
    container = context.containers[container_name]
    return helpers.exec(container, cmd)


@when('we kill "{service}" in container "{name}" with signal "{signal}"')
def step_kill_service(context, service, name, signal):
    ensure_exec(context, name, 'pkill --signal %s %s' % (signal, service))


@when('we gracefully stop "{service}" in container "{name}"')
def step_stop_service(context, service, name):
    if service == 'greenplum':
        gp_datadir = _container_get_pgdata(context, name)
        code, output = ensure_exec(
            context, name, f"sudo -u gpadmin /opt/greenplum-db-6/bin/pg_ctl stop -s -m fast -w -t 60 -D {gp_datadir}"
        )
        assert code == 0, f'Could not stop greenplum: {output}'
    else:
        ensure_exec(context, name, 'supervisorctl stop %s' % service)


def _parse_pgdata(lsclusters_output):
    """
    Parse pgdata from 1st row
    """
    for row in lsclusters_output.split('\n'):
        if not row:
            continue
        _, _, _, _, _, pgdata, _ = row.split()
        return pgdata


def _container_get_pgdata(context, name):
    """
    Get pgdata in container by name
    """
    code, clusters_str = ensure_exec(context, name, '/opt/greenplum-db-6/bin/gp_lsclusters')
    assert code == 0, f'Could not list clusters: {clusters_str}'
    return _parse_pgdata(clusters_str)


@when('we start "{service}" in container "{name}"')
def step_start_service(context, service, name):
    if service == 'postgres':
        pgdata = _container_get_pgdata(context, name)
        code, output = ensure_exec(context, name, f'sudo -u postgres /usr/bin/postgresql/pg_ctl start -D {pgdata}')
        assert code == 0, f'Could not start postgres: {output}'
    else:
        ensure_exec(context, name, 'supervisorctl start %s' % service)


@when('we stop container "{name}"')
def step_stop_container(context, name):
    context.execute_steps(
        """
        When we kill container "{name}" with signal "SIGTERM"
        Then container "{name}" has status "exited"
    """.format(
            name=name
        )
    )


@when('we start container "{name}"')
def step_start_container(context, name):
    container = context.containers[name]
    container.reload()
    status = helpers.container_get_status(container)
    assert status == 'exited', 'Unexpected container state "{state}", expected "exited"'.format(state=status)
    container.start()
    container.reload()


@when('we start container "{name}" and start gpsync')
def step_start_container_and_gpsync(context, name):
    context.execute_steps(
        """
        When we start container "{name}"
        And we start "gpsync" in container "{name}"
        """.format(
            name=name
        )
    )


@when('we disconnect from network container "{name}"')
def step_disconnect_container(context, name):
    networks = context.compose['services'][name]['networks']
    container = context.containers[name]
    for netname in networks:
        context.networks[netname].disconnect(container)


@when('we connect to network container "{name}"')
@when('we connect to network container "{name}" and do nothing')
def step_connect_container(context, name):
    networks = context.compose['services'][name]['networks']
    container = context.containers[name]
    for netname, network in networks.items():
        context.networks[netname].connect(container, **network)


@then('we fail')
def step_fail(_):
    raise AssertionError('You asked - we failed')


@when('we wait "{interval:f}" seconds')
def step_sleep(_, interval):
    time.sleep(interval)


@when('we wait until "{interval:f}" seconds to failover of "{container_name}" left in zookeeper "{zk_name}"')
def step_sleep_until_failover_cooldown(context, interval, container_name, zk_name):
    last_failover_ts = helpers.get_zk_value(context, zk_name, '/gpsync/postgresql/last_failover_time')
    assert last_failover_ts is not None, 'last_failover_ts should not be "None"'
    last_failover_ts = float(last_failover_ts)

    timeout = config.getint(context, container_name, 'gpsync.conf', 'replica', 'min_failover_timeout')
    now = time.time()
    wait_duration = (last_failover_ts + timeout) - now - interval
    assert wait_duration >= 0, 'we can\'t wait negative amount of time'
    time.sleep(wait_duration)


@when('we disable archiving in "{name}"')
def step_disable_archiving(context, name):
    container = context.containers[name]
    db = Postgres(host=helpers.container_get_host(), port=helpers.container_get_tcp_port(container, 5432))
    db.disable_archiving()


@when('we switch wal in "{name}" "{times:d}" times')
def switch_wal(context, name, times):
    container = context.containers[name]
    db = Postgres(host=helpers.container_get_host(), port=helpers.container_get_tcp_port(container, 5432))
    context.wals = []
    for _ in range(times):
        context.wals.append(db.switch_and_get_wal())
        time.sleep(1)


@then('wals present on backup "{name}"')
@helpers.retry_on_assert
def check_wals(context, name):
    container = context.containers[name]
    for wal in context.wals:
        assert helpers.container_check_file_exists(
            container, '/archive/{wal}'.format(wal=wal)
        ), 'wal "{wal}" not present '.format(wal=wal)


@when('we run following command on host "{name}"')
def step_host_run_command(context, name):
    context.last_exit_code, context.last_output = ensure_exec(context, name, context.text)


@then('command exit with return code "{code:d}"')
def step_command_return_code(context, code):
    assert (
        code == context.last_exit_code
    ), f'Expected "{code}", got "{context.last_exit_code}", output was "{context.last_output}"'


@then('command result is following output')
def step_command_output_exact(context):
    assert context.text == context.last_output, f'Expected "{context.text}", got "{context.last_output}"'


@then('command result contains following output')
def step_command_output_contains(context):
    assert context.text in context.last_output, f'Expected "{context.text}" not found in got "{context.last_output}"'


@when('we promote host "{name}"')
@helpers.retry_on_assert
def promote(context, name):
    container = context.containers[name]
    helpers.promote_host(container)


@when('we make switchover task with params "{params}" in container "{name}"')
def set_switchover_task(context, params, name):
    container = context.containers[name]
    if params == "None":
        params = ""
    helpers.set_switchover(container, params)


@then('gpsync in container "{name}" is connected to zookeeper')
@helpers.retry_on_assert
def step_check_gpsync_zk_connection(context, name):
    container = context.containers[name]
    _, output = container.exec_run("bash -c '/usr/bin/lsof -i -a -p `supervisorctl pid gpsync`'", privileged=True)
    gpsync_conns = []
    for line in output.decode().split('\n'):
        conns = line.split()[8:]
        if '(ESTABLISHED)' in conns:
            gpsync_conns += [c.split('->')[1].rsplit(':', 1) for c in conns if c != '(ESTABLISHED)']
    gpsync_zk_conns = [c for c in gpsync_conns if 'zookeeper' in c[0] and '2181' == c[1]]
    assert gpsync_zk_conns, "gpsync in container {name} is not connected to zookeper".format(name=name)


@then('"{x:d}" containers are replicas of "{master_name}" within "{sec:f}" seconds')
def step_x_containers_are_replicas_of(context, x, master_name, sec):
    timeout = time.time() + sec
    while time.time() < timeout:
        replicas_count = 0
        for container_name in context.containers:
            if 'postgres' not in container_name:
                continue
            try:
                assert_container_is_replica(context, container_name, master_name)
            except AssertionError:
                # this container is not a replica of master, ok
                pass
            else:
                replicas_count += 1
        if replicas_count == x:
            return
        time.sleep(context.interval)
    assert False, "{x} containers are not replicas of {master}".format(x=x, master=master_name)


@then('at least "{x}" postgresql instances are running for "{interval:f}" seconds')
def step_x_postgresql_are_running(context, x, interval):
    start_time = time.time()
    while time.time() < start_time + interval:
        x = int(x)
        running_count = 0
        for container in context.containers.values():
            try:
                db = Postgres(host=helpers.container_get_host(), port=helpers.container_get_tcp_port(container, 6432))
            except AssertionError:
                # ok, this db is not running right now
                pass
            else:
                if db.ping():
                    running_count += 1
        assert (
            running_count >= x
        ), "postgresql should be running in " + "{x} containers, but it is running in {y} containers".format(
            x=x, y=running_count
        )


def get_minimal_simultaneously_running_count(state_changes, cluster_size):
    running_count = 0
    is_cluster_completed = False
    minimal_running_count = None
    for change in state_changes:
        if change.new_state == helpers.DBState.shut_down:
            running_count -= 1
            if is_cluster_completed:
                minimal_running_count = min(minimal_running_count, running_count)
        elif change.new_state == helpers.DBState.working:
            running_count += 1
            if running_count == cluster_size:
                is_cluster_completed = True
                minimal_running_count = cluster_size
    return minimal_running_count


@then('container "{name}" is in sync group')
@helpers.retry_on_assert
def step_container_is_in_sync_group(context, name):
    container = context.containers[name]
    fqdn = helpers.container_get_fqdn(container)
    context.execute_steps(
        f'''
        Then zookeeper "zookeeper1" has holder "{fqdn}" for lock "/gpsync/greenplum/sync_replica"
    '''
    )
    assert zk.has_subset_of_values(
        context,
        'zookeeper1',
        '/gpsync/greenplum/replics_info',
        {
            fqdn: {
                'state': 'streaming',
                'sync_state': 'sync',
            }
        },
    )


@then('quorum replication is in normal state')
def step_quorum_replication_is_in_normal_state(context):
    pass


@then('sync replication is in normal state')
def step_single_sync_replication_is_in_normal_state(context):
    pass


@then('at least "{x}" postgresql instances were running simultaneously during test')
def step_x_postgresql_were_running_simultaneously(context, x):
    x = int(x)
    state_changes = []
    cluster_size = 0
    for (name, container) in context.containers.items():
        if 'postgres' not in name:
            continue
        cluster_size += 1
        log_stream = helpers.container_get_filestream(container, "/var/log/postgresql/postgresql.log")
        logs = list(map(lambda line: line.decode('u8'), log_stream))
        state_changes.extend(helpers.extract_state_changes_from_postgresql_logs(logs))
    state_changes = sorted(state_changes)
    min_running = get_minimal_simultaneously_running_count(state_changes, cluster_size)
    assert (
        min_running >= x
    ), "postgresql had to be running in " + "{x} containers, but it was running in {y} containers".format(
        x=x, y=min_running
    )


@when('we set value "{value}" for option "{option}" in section "{section}" in gpsync config in container "{name}"')
def step_change_gpsync_option(context, value, option, section, name):
    container = context.containers[name]
    conffile = 'gpsync.conf'
    confobj = config.fromfile(conffile, helpers.container_get_conffile(container, conffile))
    confobj.merge({section: {option: value}})
    helpers.container_inject_config(container, conffile, confobj)


@when('we set value "{value}" for option "{option}" in "{conffile}" config in container "{name}"')
def step_change_option(context, value, option, conffile, name):
    container = context.containers[name]
    confobj = config.fromfile(conffile, helpers.container_get_conffile(container, conffile))
    confobj.merge({option: value})
    helpers.container_inject_config(container, conffile, confobj)


@when('we restart "{service}" in container "{name}"')
def step_restart_service(context, service, name):
    if service == 'greenplum':
        gp_datadir = _container_get_pgdata(context, name)
        code, output = ensure_exec(
            context, name, f"sudo -u gpadmin /opt/greenplum-db-6/bin/pg_ctl restart -s -m fast -w -t 60 -D {gp_datadir}"
        )
        assert code == 0, f'Could not restart postgres: {output}'
    else:
        ensure_exec(context, name, f'supervisorctl restart {service}')


def get_postgres_start_time(context, name):
    container = context.containers[name]
    try:
        postgres = Postgres(host=helpers.container_get_host(), port=helpers.container_get_tcp_port(container, 5432))
        return postgres.get_start_time()
    except psycopg2.Error as error:
        raise AssertionError(error.pgerror)


@when('we remember postgresql start time in container "{name}"')
def step_remember_pg_start_time(context, name):
    context.pg_start_time[name] = get_postgres_start_time(context, name)


@then('postgresql in container "{name}" {restarted:WasOrNot} restarted')
def step_was_pg_restarted(context, name, restarted):
    if restarted:
        assert get_postgres_start_time(context, name) != context.pg_start_time[name]
    else:
        assert get_postgres_start_time(context, name) == context.pg_start_time[name]


@then('greenplum in container "{name}" {rewinded:WasOrNot} rewinded')
def step_was_pg_rewinded(context, name, rewinded):
    container = context.containers[name]
    actual_rewinded = helpers.container_file_exists(container, '/tmp/rewind_called')
    assert rewinded == actual_rewinded


@then('container "{name}" is replaying WAL')
@helpers.retry_on_assert
def step_container_replaying_wal(context, name):
    container = context.containers[name]
    try:
        db = Postgres(host=helpers.container_get_host(), port=helpers.container_get_tcp_port(container, 5432))
        assert not db.is_wal_replay_paused()
    except psycopg2.Error as error:
        raise AssertionError(error.pgerror)


@when('we pause replaying WAL in container "{name}"')
def step_container_pause_replaying_wal(context, name):
    container = context.containers[name]
    db = Postgres(host=helpers.container_get_host(), port=helpers.container_get_tcp_port(container, 5432))
    db.wal_replay_pause()
