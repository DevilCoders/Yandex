"""
ElasticSearch related steps
"""
import json

from behave import given, then, when
from hamcrest import assert_that, equal_to, has_item

from tests.helpers import elasticsearch
from tests.helpers.step_helpers import get_step_data, step_require


@given('set user defined Elasticsearch version')
@step_require('cluster_config')
def step_user_defined_version(context):
    context.cluster_config['config_spec']['version'] = context.config.userdata.get('version', '')


@when('we index the following documents to "{index_name}"')
@step_require('cluster', 'hosts')
def step_index_documents(context, index_name):
    docs = json.loads(context.text)
    elasticsearch.index_documents(context, index_name, docs)


@then('number of documents in "{index_name}" equals to "{num_docs:n}"')
@step_require('cluster', 'hosts')
def step_count_documents(context, index_name, num_docs):
    cnt = elasticsearch.count_documents(context, index_name)
    assert_that(cnt, equal_to(num_docs))


@when('we search "{index_name}" with the following query on all hosts')
@step_require('cluster', 'hosts')
def step_search_documents(context, index_name):
    query = json.loads(context.text)
    responses = elasticsearch.search_documents(context, index_name, query)
    context.responses = responses


@then('result contains the following documents on all hosts')
@step_require('responses')
def step_result_contains(context):
    expected = {}
    for resp in json.loads(context.text):
        _id = resp['_id']
        del resp['_id']
        expected[_id] = resp

    for resp in context.responses:
        assert_that(resp, equal_to(expected))


@then('heap size of "{role}" node equals to "{jvm_heap}" bytes')
@step_require('hosts')
def step_jvm_heap_size(context, role, jvm_heap):
    assert_that(elasticsearch.get_jvm_heap_in_bytes(context, role), equal_to(int(jvm_heap)))


@then('static setting "{set_name}" of "{role}" node equals to "{set_val}"')
@step_require('hosts')
def step_static_setting(context, set_name, role, set_val):
    assert_that(elasticsearch.get_static_setting(context, set_name, role), equal_to(set_val))


@then('number of "{role}" nodes equals to "{nodes_num}"')
@step_require('hosts')
def step_count_nodes(context, role, nodes_num):
    assert_that(elasticsearch.count_nodes(context, role), equal_to(int(nodes_num)))


@then('specified plugins installed on all nodes')
def step_plugins_installed(context):
    nodes = elasticsearch.nodes_plugins(context)
    for row in context.table:
        for plugins in nodes:
            assert_that(plugins, has_item(row['plugin_name']))


@then('all nodes are of version')
def step_nodes_of_version(context):
    params = get_step_data(context)
    nodes = elasticsearch.nodes_versions(context)
    for node in nodes:
        assert_that(node['version'], equal_to(params['version']), json.dumps(nodes, indent=4))
