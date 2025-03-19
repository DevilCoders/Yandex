import logging

from behave import given, then, when

from tests.helpers.metastore_cluster import MetastoreCluster
from tests.helpers.step_helpers import print_request_on_fail, step_require


def get_metastore_cluster(context):
    if not hasattr(context, 'metastore_cluster'):
        logging.debug('Creating new MetastoreCluster object')
        context.metastore_cluster = MetastoreCluster(context)
    return context.metastore_cluster


@when('we try to create metastore cluster "{cluster_name}" with following config overrides')
@when('we try to create metastore cluster "{cluster_name}"')
@step_require('cluster_config', 'folder', 'cluster_type')
@print_request_on_fail
def step_create_cluster(context, cluster_name):
    get_metastore_cluster(context).create_cluster(cluster_name)


@when('we test metastore')
@step_require('cluster_config', 'folder', 'cluster_type')
@print_request_on_fail
def step_create_cluster(context):
    result = get_metastore_cluster(context).test_metastore()
    assert result


@when('we try to remove metastore cluster')
@when('we try to delete metastore cluster')
@step_require('cid')
@print_request_on_fail
def step_delete_cluster(context):
    get_metastore_cluster(context).delete_cluster()


@given('metastore cluster "{cluster_name}" is up and running')
@then('metastore cluster "{cluster_name}" is up and running')
@step_require('folder')
def step_cluster_is_running(context, cluster_name):
    get_metastore_cluster(context).load_cluster_into_context(name=cluster_name)
    assert context.cluster['status'] == 'RUNNING', f"Cluster has unexpected status {context.cluster['status']}"


@given('metastore cluster "{cluster_name}" exists')
@then('metastore cluster "{cluster_name}" exists')
@step_require('folder')
def step_cluster_exists(context, cluster_name):
    get_metastore_cluster(context).load_cluster_into_context(name=cluster_name)
