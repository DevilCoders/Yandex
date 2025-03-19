"""
Steps related to Greenplum cluster.
"""

import logging

from behave import given, then, when

from tests.helpers.greenplum_cluster import GreenplumCluster
from tests.helpers.step_helpers import get_step_data, print_request_on_fail, step_require


def get_greenplum_cluster(context):
    if not hasattr(context, 'greenplum_cluster'):
        logging.debug('Creating new GreenplumCluster object')
        context.greenplum_cluster = GreenplumCluster(context)
    return context.greenplum_cluster


@when('we try to create greenplum cluster "{cluster_name}" with following config overrides')
@when('we try to create greenplum cluster "{cluster_name}"')
@step_require('cluster_config', 'folder', 'cluster_type')
@print_request_on_fail
def step_create_cluster(context, cluster_name):
    get_greenplum_cluster(context).create_cluster(cluster_name)


@when('we try to update greenplum cluster')
@step_require('cid')
@print_request_on_fail
def step_update_cluster(context):
    get_greenplum_cluster(context).update_cluster()


@when('we try to add hosts to greenplum cluster')
@step_require('cid')
@print_request_on_fail
def step_add_hosts_to_cluster(context):
    get_greenplum_cluster(context).add_hosts()


@when('we try to remove greenplum cluster')
@when('we try to delete greenplum cluster')
@step_require('cid')
@print_request_on_fail
def step_delete_cluster(context):
    get_greenplum_cluster(context).delete_cluster()


@given('greenplum cluster "{cluster_name}" is up and running')
@then('greenplum cluster "{cluster_name}" is up and running')
@step_require('folder')
def step_cluster_is_running(context, cluster_name):
    get_greenplum_cluster(context).load_cluster_into_context(name=cluster_name)
    assert context.cluster['status'] == 'RUNNING', f"Cluster has unexpected status {context.cluster['status']}"


@given('greenplum cluster "{cluster_name}" exists')
@then('greenplum cluster "{cluster_name}" exists')
@step_require('folder')
def step_cluster_exists(context, cluster_name):
    get_greenplum_cluster(context).load_cluster_into_context(name=cluster_name)
