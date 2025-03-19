"""
Steps related to SQLServer cluster.
"""

import logging
import time
import humanfriendly
from behave import given, when, then
from hamcrest import assert_that, equal_to

from internal_api import find_host
from tests.helpers import go_internal_api, sqlserver
from tests.helpers.step_helpers import (
    get_step_data,
    get_time,
    print_request_on_fail,
    step_require,
    store_grpc_exception,
)
from tests.helpers.sqlserver_cluster import SqlServerCluster

QUERY_CHECK_DB_EXISTS = "SELECT state_desc FROM sys.databases where name = '{db_name}'"

STD_PWD = "12345678"

REPL_DB = 'testdb'


def get_sqlserver_cluster(context) -> SqlServerCluster:
    if not hasattr(context, 'sqlserver_cluster'):
        logging.debug('Creating new SqlServer object')
        context.sqlserver_cluster = SqlServerCluster(context)
    return context.sqlserver_cluster


def extract_backup_name(backup) -> str:
    s = backup['id']
    return s[s.find(':') + 1 :]


@when('we try to create sqlserver cluster "{cluster_name}" with following config overrides')
@when('we try to create sqlserver cluster "{cluster_name}"')
@step_require('cluster_config', 'folder', 'cluster_type')
@print_request_on_fail
def step_create_cluster(context, cluster_name):
    get_sqlserver_cluster(context).create_cluster(cluster_name)


@when('we try to update sqlserver cluster')
@step_require('cid')
@store_grpc_exception
def step_update_cluster(context):
    get_sqlserver_cluster(context).update_cluster()


@when('we try to remove sqlserver cluster')
@when('we try to delete sqlserver cluster')
@step_require('cid')
@print_request_on_fail
def step_delete_cluster(context):
    get_sqlserver_cluster(context).delete_cluster()


@given('we are selecting "{cluster_name}" sqlserver cluster for following steps')
@given('sqlserver cluster "{cluster_name}" is up and running')
@then('sqlserver cluster "{cluster_name}" is up and running')
@step_require('folder')
def step_cluster_is_running(context, cluster_name):
    get_sqlserver_cluster(context).load_cluster_into_context(name=cluster_name)
    assert context.cluster['status'] == 'RUNNING', f"Cluster has unexpected status {context.cluster['status']}"


@when('we create backup for selected sqlserver cluster via GRPC')
@step_require('cid')
def step_create_backup(context):
    get_sqlserver_cluster(context).create_backup(context.cid)


@when('we remember last cluster backup name')
@step_require('cid')
def step_remember_last_backup_name(context):
    latest = get_sqlserver_cluster(context).get_latest_backup(context.cid)
    backup_name = extract_backup_name(latest)

    setattr(context, 'last_backup_name', backup_name)


@then('last cluster backup name greater than saved')
@step_require('cid')
def step_cluster_backup_name_greater_than_saved(context):
    latest = get_sqlserver_cluster(context).get_latest_backup(context.cid)
    backup_name = extract_backup_name(latest)

    assert backup_name > context.last_backup_name


@when('we restore sqlserver cluster from latest backup from cluster "{cluster_name:Param}" via GRPC')
@step_require('config_type', 'cid')
def step_restore_cluster(context, cluster_name):
    get_sqlserver_cluster(context).restore_cluster()


@when('we try to create database "{db_name}"')
@step_require('cluster_config', 'folder', 'cluster_type')
def step_create_database(context, db_name):
    get_sqlserver_cluster(context).create_database(db_name)


@when('we try to restore database "{from_name}" as "{db_name}"')
@when('we try to restore database "{from_name}" as "{db_name}" at "{time_key}"')
@step_require('cluster_config', 'folder', 'cluster_type')
def step_restore_database(context, from_name, db_name, time_key=None):
    time = get_time(context, time_key) if time_key is not None else None
    get_sqlserver_cluster(context).restore_database(db_name, from_name, 'LATEST', time)


@when('we try to delete database "{db_name}"')
@step_require('cluster_config', 'folder', 'cluster_type')
def step_delete_database(context, db_name):
    get_sqlserver_cluster(context).delete_database(db_name)


@when('we try to create database "{db_name}" owner user "{user_name}" with standard pwd')
@step_require('cluster_config', 'folder', 'cluster_type')
def step_create_user_owner(context, db_name, user_name):
    get_sqlserver_cluster(context).create_user(user_name, STD_PWD, db_name, ["DB_OWNER"])


@when('we try to create user "{user_name}" with standard password')
@when('we try to create user "{user_name}" with "{password}" password')
@when('we try to create user "{user_name}" with standard password and "{role}" role in database "{db_name}"')
@when('we try to create user "{user_name}" with "{password}" password and "{role}" role in database "{db_name}"')
@step_require('cluster_config', 'folder', 'cluster_type')
def step_create_user(context, user_name, password=STD_PWD, role=None, db_name=None):
    if role is not None:
        role = [role]
    get_sqlserver_cluster(context).create_user(user_name, password, db_name, role)


@when('we try to delete user "{user_name}"')
@step_require('cluster_config', 'folder', 'cluster_type')
def step_delete_user(context, user_name):
    get_sqlserver_cluster(context).delete_user(user_name)


@when('we try to grant "{role}" role in database "{db_name}" to user "{user_name}"')
@step_require('cluster_config', 'folder', 'cluster_type')
def step_grant_permission(context, role, db_name, user_name):
    get_sqlserver_cluster(context).grant_permission(user_name, db_name, [role])


@when('we try to revoke "{role}" role in database "{db_name}" from user "{user_name}"')
@step_require('cluster_config', 'folder', 'cluster_type')
def step_revoke_permission(context, role, db_name, user_name):
    get_sqlserver_cluster(context).revoke_permission(user_name, db_name, [role])


@then('sqlserver user "{user_name}" can connect to "{db_name}"')
@then('sqlserver user "{user_name}" can {NOT} connect to "{db_name}"')
@step_require('cluster_config', 'folder', 'cluster_type')
def step_can_connect(context, user_name, db_name, NOT=False):
    get_sqlserver_cluster(context).load_cluster_hosts_into_context()
    ok = None
    try:
        conn = sqlserver.connect(
            context=context,
            master=True,
            cluster_config=context.cluster_config,
            hosts=context.hosts,
            user=user_name,
            password=get_user_pwd(context, user_name),
            dbname=db_name,
        )
        ok = True
    except Exception as err:
        if sqlserver.is_login_failed_error(err):
            ok = False
        else:
            raise err
    if NOT:
        assert_that(ok, equal_to(False), f'user "{user_name}" unexpectedly logined to "{db_name}"')
    else:
        assert_that(ok, equal_to(True), f'user "{user_name}" failed to login to "{db_name}"')


@then('sqlserver user "{user_name}" can create table in "{db_name}"')
@then('sqlserver user "{user_name}" can {NOT} create table in "{db_name}"')
@step_require('cluster_config', 'folder', 'cluster_type')
def step_can_create_table(context, user_name, db_name, NOT=False):
    get_sqlserver_cluster(context).load_cluster_hosts_into_context()
    conn = sqlserver.connect(
        context=context,
        master=True,
        cluster_config=context.cluster_config,
        hosts=context.hosts,
        user=user_name,
        password=get_user_pwd(context, user_name),
        dbname=db_name,
    )
    cur = conn.cursor()
    ok = None
    try:
        cur.execute('CREATE TABLE [access_test_table] (id INT PRIMARY KEY)')
        conn.commit()
        ok = True
    except Exception as err:
        if sqlserver.is_access_violation_error(err):
            ok = False
        else:
            raise err

    if NOT:
        assert_that(ok, equal_to(False), f'user "{user_name}" unexpectedly created table in "{db_name}"')
    else:
        assert_that(ok, equal_to(True), f'user "{user_name}" failed to create table in "{db_name}"')


@then('sqlserver user "{user_name}" can select from "{db_name}"')
@then('sqlserver user "{user_name}" can {NOT} select from "{db_name}"')
@step_require('cluster_config', 'folder', 'cluster_type')
def step_can_select(context, user_name, db_name, NOT=False):
    get_sqlserver_cluster(context).load_cluster_hosts_into_context()
    conn = sqlserver.connect(
        context=context,
        master=True,
        cluster_config=context.cluster_config,
        hosts=context.hosts,
        user=user_name,
        password=get_user_pwd(context, user_name),
        dbname=db_name,
    )
    cur = conn.cursor()
    ok = None
    try:
        cur.execute('SELECT count(*) FROM access_test_table')
        ok = True
    except Exception as err:
        if sqlserver.is_access_violation_error(err):
            ok = False
        else:
            raise err
    if NOT:
        assert_that(ok, equal_to(False), f'user "{user_name}" unexpectedly selected from table in "{db_name}')
    else:
        assert_that(ok, equal_to(True), f'user "{user_name}" failed to select from table in "{db_name}')


@then('sqlserver user "{user_name}" can insert into "{db_name}"')
@then('sqlserver user "{user_name}" can {NOT} insert into "{db_name}"')
@step_require('cluster_config', 'folder', 'cluster_type')
def step_can_insert(context, user_name, db_name, NOT=False):
    get_sqlserver_cluster(context).load_cluster_hosts_into_context()
    conn = sqlserver.connect(
        context=context,
        master=True,
        cluster_config=context.cluster_config,
        hosts=context.hosts,
        user=user_name,
        password=get_user_pwd(context, user_name),
        dbname=db_name,
    )
    cur = conn.cursor()
    ok = None
    try:
        cur.execute('INSERT INTO access_test_table(id) VALUES(1)')
        conn.commit()
        ok = True
    except Exception as err:
        if sqlserver.is_access_violation_error(err):
            ok = False
        else:
            raise err
    if NOT:
        assert_that(ok, equal_to(False), f'user "{user_name}" unexpectedly inserted into "{db_name}"')
    else:
        assert_that(ok, equal_to(True), f'user "{user_name}" failed to insert into "{db_name}"')


@then('sqlserver database "{db_name}" exists')
@step_require('cluster_config', 'folder', 'cluster_type')
def step_db_exists(context, db_name):
    expected_res = ['ONLINE']
    ensure_query_within_timeout(
        context=context,
        query=QUERY_CHECK_DB_EXISTS.format(db_name=db_name),
        master=True,
        expected_res=expected_res,
    )

    assert_that(context.query_result['result'], equal_to(expected_res))


@then('sqlserver database "{db_name}" not exist')
@step_require('cluster_config', 'folder', 'cluster_type')
def step_db_not_exists(context, db_name):
    expected_res = []
    ensure_query_within_timeout(
        context=context,
        query=QUERY_CHECK_DB_EXISTS.format(db_name=db_name),
        master=True,
        expected_res=expected_res,
    )

    assert_that(context.query_result['result'], equal_to(expected_res))


@when('we try to export sqlserver database backup')
def step_export_database_backup(context):
    req = get_step_data(context)
    get_sqlserver_cluster(context).export_database_backup(req)


@when('we try to import sqlserver database backup')
def step_import_database_backup(context):
    req = get_step_data(context)
    get_sqlserver_cluster(context).import_database_backup(req)


@when('we add test data "{data:w}" on cluster "{cluster_name}" in "{db_name}"')
def step_we_add_test_data(context, data, cluster_name, db_name):
    get_sqlserver_cluster(context).load_cluster_hosts_into_context()
    insert_query = """
        if not exists (select * from sys.tables where name = 'data_test_table')
            create table [data_test_table] (d varchar(256) primary key);
        insert into data_test_table values ('{data}')
    """.format(
        data=data
    )
    sqlserver.db_query(
        context=context,
        query=insert_query,
        master=True,
        cluster_config=context.cluster_config,
        hosts=context.hosts,
        dbname=db_name,
        ignore_results=True,
    )


@then('test data "{data:w}" exists on cluster "{cluster_name}" in "{db_name}"')
def step_test_data_exists(context, data, cluster_name, db_name):
    get_sqlserver_cluster(context).load_cluster_hosts_into_context()
    check_query = """
        select top 1 d from data_test_table where d = '{data}'
    """.format(
        data=data
    )
    sqlserver.db_query(
        context=context,
        query=check_query,
        master=True,
        cluster_config=context.cluster_config,
        hosts=context.hosts,
        dbname=db_name,
    )
    assert (
        hasattr(context, 'query_result')
        and 'result' in context.query_result
        and context.query_result['result'][0] == data
    ), "test data not found in data_test_table"


# first we need to set server_roles via API
@then('hadr replication on cluster "{cluster_name}" is running')
@then('hadr replication on cluster "{cluster_name}" is running within "{timeout}"')
def step_hadr_replication_is_running(context, cluster_name, timeout=None):
    get_sqlserver_cluster(context).load_cluster_hosts_into_context()
    check_query = """
        select min(coalesce(synchronization_state, 0)), max(coalesce(synchronization_state, 0)),
               min(coalesce(synchronization_health, 0)), max(coalesce(synchronization_health, 0))
        from sys.dm_hadr_database_replica_states st
        right join sys.databases dbs
           on dbs.database_id = st.database_id
        where dbs.name not in ('master', 'tempdb', 'model', 'msdb')
    """
    conn = sqlserver.connect(
        context=context,
        master=True,
        cluster_config=context.cluster_config,
        hosts=context.hosts,
    )
    cur = conn.cursor()
    if timeout is None:
        cur.execute(check_query)
        row = cur.fetchone()
        assert row is not None and tuple(row) == (2, 2, 2, 2), "unexpected database_replica_states record: {}".format(
            list(row)
        )
        return
    deadline = time.time() + humanfriendly.parse_timespan(timeout)
    last_exception = None
    while time.time() < deadline:
        last_exception = None
        try:
            cur.execute(check_query)
            row = cur.fetchone()
            assert row is not None and tuple(row) == (
                2,
                2,
                2,
                2,
            ), "unexpected database_replica_states record: {}".format(list(row))
        except Exception as e:
            last_exception = e
            time.sleep(1)
    if conn is not None:
        conn.close()
    if last_exception is not None:
        raise last_exception


def get_next_counter(context):
    name = '_sqlserver_autoinc_counter'
    try:
        counter = getattr(context, name)
    except (AttributeError, KeyError):
        counter = int(time.time())
    counter += 1
    setattr(context, name, counter)
    return int(counter)


def get_user_pwd(context, user_name):
    user_specs = context.cluster_config['userSpecs']
    for user in user_specs:
        if user['name'] == user_name:
            return user['password']

    return STD_PWD


def ensure_query_within_timeout(context, query, expected_res, timeout=None, master=False, **kwargs):
    get_sqlserver_cluster(context).load_cluster_hosts_into_context()
    if timeout is not None:
        deadline = time.time() + humanfriendly.parse_timespan(timeout)
        while time.time() < deadline:
            sqlserver.db_query(
                context=context,
                query=query,
                master=master,
                cluster_config=context.cluster_config,
                hosts=context.hosts,
                **kwargs,
            )
            result = context.query_result['result']
            if result == expected_res:
                return
            else:
                time.sleep(1)
    else:
        sqlserver.db_query(
            context=context,
            query=query,
            master=master,
            cluster_config=context.cluster_config,
            hosts=context.hosts,
            **kwargs,
        )


@when('we attempt to failover sqlserver cluster "{cluster_name}" via GRPC')
@when('we attempt to failover sqlserver cluster "{cluster_name}" to host in geo "{geo}" via GRPC')
@step_require('cid')
def step_failover_backup(context, cluster_name: str, geo=""):
    get_sqlserver_cluster(context).load_cluster_hosts_into_context()
    host = find_host(context, geo=geo)
    get_sqlserver_cluster(context).failover(context.cid, host)
