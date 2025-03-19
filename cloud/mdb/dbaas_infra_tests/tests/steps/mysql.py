"""
MySQL related steps
"""
import time

import pymysql
import yaml
from behave import then, when
from hamcrest import assert_that, equal_to
from retrying import retry

from tests.helpers import docker, internal_api
from tests.helpers.step_helpers import get_step_data, step_require


@then('all changes in mysql "{cluster_name}" are replicated within "{seconds}" seconds')
def step_mysql_changes_are_replicated(context, cluster_name, seconds):
    internal_api.load_cluster_into_context(context, cluster_name)
    internal_api.mysql_query(context, query="SELECT @@gtid_executed AS gtid", master=True)
    master_gtid = context.query_result['result'][0]['gtid']
    sql = '''
        SELECT
            GTID_SUBSET('{gtid}', @@gtid_executed) AS ok,
            @@gtid_executed AS gtid
    '''.format(gtid=master_gtid)
    deadline = time.time() + int(seconds)
    while time.time() < deadline:
        failed_host = None
        failed_gtid = None
        for host in context.hosts:
            geo = host.get('zoneId')
            internal_api.mysql_query(context, query=sql, geo=geo)
            if not int(context.query_result['result'][0]['ok']):
                failed_host = geo
                failed_gtid = context.query_result['result'][0]['gtid']
        if failed_host is None:
            return
        time.sleep(1)
    assert False, 'host "{host}" has {gtid}, while master has {master_gtid}'.format(
        host=failed_host, gtid=failed_gtid, master_gtid=master_gtid)


@then('we are {ability} to log in mysql "{cluster_name}" with following ' 'parameters')
@then('we are {ability} to log in mysql with following ' 'parameters')
@step_require('cluster_config', 'folder')
@retry(wait_fixed=1000, stop_max_attempt_number=10)
def step_mysql_user_connect(context, ability, cluster_name=None):
    if cluster_name is not None:
        internal_api.load_cluster_into_context(context, cluster_name)

    user_creds = yaml.load(context.text or '') or {}
    try:
        internal_api.mysql_query(context, query="SHOW TABLES", **user_creds)
        assert_that('able', equal_to(ability), 'login successful')
    except pymysql.Error:
        assert_that('unable', equal_to(ability), 'login failed')


@when('we execute command on mysql master in "{cluster_name}"')
@when('we execute command on mysql master in "{cluster_name}" as user "{user}"')
def step_run_command_on_host(context, cluster_name, user='root'):
    internal_api.load_cluster_into_context(context, cluster_name)
    internal_api.mysql_query(context, query="SELECT @@hostname AS host", master=True)
    master_fqdn = context.query_result['result'][0]['host']
    cmd = context.text.strip()
    # container name is the same as fqdn here
    retcode, output = docker.run_command(master_fqdn, cmd, user=user)
    context.command_retcode = retcode
    context.command_output = output.decode()


@then('MySQL major version result is {matches:QueryResultMatcher}')
@step_require('query_result')
def step_check_query_result(context, matches):
    assert_that(
        int(get_step_data(context)[0]['version']),
        matches(int(context.query_result['result'][0]['Value'].split('.')[0])))


@then('all hosts in mysql "{cluster_name}" are online')
def step_mysql_hosts_are_online(context, cluster_name):
    internal_api.load_cluster_into_context(context, cluster_name)
    sql = 'SELECT @@offline_mode AS offline'
    failed_host = None
    for host in context.hosts:
        geo = host.get('zoneId')
        internal_api.mysql_query(context, query=sql, geo=geo)
        if int(context.query_result['result'][0]['offline']):
            failed_host = geo
    assert failed_host is None, 'host "{host}" is offline'.format(host=failed_host)
