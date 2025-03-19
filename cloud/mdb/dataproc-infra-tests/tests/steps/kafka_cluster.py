"""
Steps related to Kafka cluster.
"""

import humanfriendly
import logging
import time

from behave import given, then, when

from tests.helpers.grpcutil.service import NotFoundError

from tests.helpers.kafka_cluster import KafkaCluster
from tests.helpers.step_helpers import get_step_data, print_request_on_fail, step_require


def get_kafka_cluster(context):
    if not hasattr(context, 'kafka_cluster'):
        logging.debug('Creating new KafkaCluster object')
        context.kafka_cluster = KafkaCluster(context)
    return context.kafka_cluster


@when('we try to read message from topic "{topic}" as user "{user}"')
def step_try_to_read(context, topic, user):
    message, errors = get_kafka_cluster(context).consume_message(user, topic)
    context.consumed_message = message
    context.kafka_errors = errors


@then('we read message from topic "{topic}" as user "{user}" and its text is "{expected_message}"')
@then('we receive message "{expected_message}" from topic "{topic}" as user "{user}" within "{timeout}"')
def step_read(context, topic, user, expected_message, timeout='10 seconds'):
    timeout = humanfriendly.parse_timespan(timeout)
    message, errors = get_kafka_cluster(context).consume_message(user, topic, timeout)
    assert message, f"failed to consume any message from topic {topic}, errors: {errors}"
    message = message.decode("utf-8")
    assert message == expected_message, f'expected message "{expected_message}" but got "{message}"'


@when('we try to write message "{message}" to topic "{topic}" as user "{user}"')
def step_try_to_write(context, message, topic, user):
    delivered, errors = get_kafka_cluster(context).produce_message(user, topic, message)
    context.message_delivered = delivered
    context.kafka_errors = errors


@given('we write message "{message}" to topic "{topic}" as user "{user}"')
def step_write(context, message, topic, user):
    step_try_to_write(context, message, topic, user)
    assert context.message_delivered, f"message was not delivered by producer"


@then('consumer fails with error {err_code}')
def step_check_consumer_error(context, err_code):
    assert context.consumed_message is None, f'expected no messages to be consumed but got "{context.consumed_message}"'
    assert_kafka_error(context.kafka_errors, err_code)


@then('producer fails with error {err_code}')
def step_check_producer_error(context, err_code):
    assert_kafka_error(context.kafka_errors, err_code)


def assert_kafka_error(kafka_errors, err_code):
    if not kafka_errors:
        assert False, f"expected error with code {err_code} but there are no errors"
    error_codes = {error.name() for error in kafka_errors}
    assert err_code in error_codes, f"expected error with code {err_code} but received only following: {error_codes}"


@when(
    'user "{user}" creates topic "{topic_name}" via Kafka Admin API with {partitions:d} partitions and {replication_factor:d} replicas and following config'
)
def step_create_topic_via_admin_api(context, user, topic_name, partitions, replication_factor):
    config = get_step_data(context)
    get_kafka_cluster(context).create_topic_via_admin_api(user, topic_name, partitions, replication_factor, config)


@when('user "{user}" updates topic "{topic_name}" to have {partitions:d} partitions and sets its config to')
def step_update_topic_via_admin_api(context, user, topic_name, partitions):
    config = get_step_data(context)
    get_kafka_cluster(context).update_topic_via_admin_api(user, topic_name, partitions, config)


@when('user "{user}" deletes topic "{topic_name}" via Kafka Admin API')
def step_delete_topic_via_admin_api(context, user, topic_name):
    get_kafka_cluster(context).delete_topic_via_admin_api(user, topic_name)


@then('wait no more than "{timeout}" until topic "{topic_name}" appears within Cloud API')
def step_wait_topic_appears(context, timeout, topic_name):
    deadline = time.time() + humanfriendly.parse_timespan(timeout)
    while time.time() < deadline:
        try:
            get_kafka_cluster(context).get_topic(topic_name)
            return
        except NotFoundError:
            time.sleep(1)
    assert False, f"timed out waiting for topic {topic_name} to appear within Cloud API"


@then('topic "{topic_name}" disappears from Cloud API within "{timeout}"')
def step_wait_topic_disappears(context, topic_name, timeout):
    deadline = time.time() + humanfriendly.parse_timespan(timeout)
    while time.time() < deadline:
        try:
            get_kafka_cluster(context).get_topic(topic_name)
            time.sleep(1)
        except NotFoundError:
            return
    assert False, f"timed out waiting for topic {topic_name} to disappear from Cloud API"


@then('topic "{topic_name}" has following params')
def step_check_topic(context, topic_name):
    expected = get_step_data(context)
    topic = get_kafka_cluster(context).get_topic(topic_name)

    partitions = expected.get('partitions')
    if partitions:
        assert partitions == int(topic['partitions']), f"Expected {partitions} partitions, got {topic['partitions']}"

    replicas = expected.get('replication_factor')
    if replicas:
        assert replicas == int(
            topic['replication_factor']
        ), f"Expected {replicas} replicas, got {topic['replication_factor']}"

    expected_config = expected.get('config')
    if expected_config:
        config_key = next(filter(lambda key: 'topic_config' in key, topic.keys()), None)
        config = topic[config_key]
        for key, expected_val in expected_config.items():
            assert key in config, f"expected config param {key} to be {expected_val} but it is missing from real config"
            real_val = config[key]
            if isinstance(expected_val, int):
                real_val = int(real_val)
            assert expected_val == real_val, f"expected config param {key} to be {expected_val}, got {real_val}"

        expected_keys = set(expected_config.keys())
        real_keys = set(config.keys())
        unexpected_keys = real_keys - expected_keys
        assert len(unexpected_keys) == 0, f"unexpected topic config params: {', '.join(unexpected_keys)}"


@then('topic "{topic_name}" has following params within "{timeout}"')
def step_check_topic_with_timeout(context, topic_name, timeout):
    deadline = time.time() + humanfriendly.parse_timespan(timeout)
    while True:
        try:
            step_check_topic(context, topic_name)
            return
        except AssertionError:
            if time.time() > deadline:
                raise
            time.sleep(1)


@then('config of Kafka broker {broker_id} has following entries')
def step_check_broker_config(context, broker_id):
    expected = get_step_data(context)
    config = get_kafka_cluster(context).get_broker_config('admin', broker_id)
    for key, val in expected.items():
        assert key in config, f"param {key} is missing from broker config"
        got_value = config[key].value
        assert got_value == val, f"expected broker param {key} to be equal to {val}, got {got_value}"


@when('we try to create kafka cluster "{cluster_name}" with following config overrides')
@when('we try to create kafka cluster "{cluster_name}"')
@step_require('cluster_config', 'folder', 'cluster_type')
@print_request_on_fail
def step_create_cluster(context, cluster_name):
    get_kafka_cluster(context).create_cluster(cluster_name)


@when('we try to stop kafka cluster "{cluster_name}"')
@step_require('cluster_config', 'folder', 'cluster_type')
@print_request_on_fail
def step_stop_cluster(context, cluster_name):
    get_kafka_cluster(context).stop_cluster(cluster_name)


@when('we try to start kafka cluster "{cluster_name}"')
@step_require('cluster_config', 'folder', 'cluster_type')
@print_request_on_fail
def step_start_cluster(context, cluster_name):
    get_kafka_cluster(context).start_cluster(cluster_name)


@when('we try to update kafka cluster')
@step_require('cid')
@print_request_on_fail
def step_update_cluster(context):
    get_kafka_cluster(context).update_cluster()


@when('we try to remove kafka cluster')
@when('we try to delete kafka cluster')
@step_require('cid')
@print_request_on_fail
def step_delete_cluster(context):
    get_kafka_cluster(context).delete_cluster()


@given('kafka cluster "{cluster_name}" is up and running')
@then('kafka cluster "{cluster_name}" is up and running')
@step_require('folder')
def step_cluster_is_running(context, cluster_name):
    get_kafka_cluster(context).load_cluster_into_context(name=cluster_name)
    assert context.cluster['status'] == 'RUNNING', f"Cluster has unexpected status {context.cluster['status']}"


@given('kafka cluster "{cluster_name}" exists')
@then('kafka cluster "{cluster_name}" exists')
@step_require('folder')
def step_cluster_exists(context, cluster_name):
    get_kafka_cluster(context).load_cluster_into_context(name=cluster_name)


@when('we try to create kafka topic')
@step_require('cid')
def step_create_topic(context):
    get_kafka_cluster(context).create_topic()


@when('we try to update kafka topic')
@step_require('cid')
def step_update_topic(context):
    get_kafka_cluster(context).update_topic()


@when('we try to delete kafka topic "{topic_name}"')
@step_require('cid')
def step_delete_topic(context, topic_name):
    get_kafka_cluster(context).delete_topic(topic_name)


@when('we try to create kafka user')
@step_require('cid')
def step_create_user(context):
    get_kafka_cluster(context).create_user()


@when('we try to update kafka user')
@step_require('cid')
def step_update_user(context):
    get_kafka_cluster(context).update_user()


@when('we try to delete kafka user "{user_name}"')
@step_require('cid')
def step_delete_user(context, user_name):
    get_kafka_cluster(context).delete_user(user_name)


@when('we try to create connector')
@step_require('cid')
def step_create_connector(context):
    get_kafka_cluster(context).create_connector()


@when('we try to delete connector "{connector_name}"')
@step_require('cid')
def step_delete_connector(context, connector_name):
    get_kafka_cluster(context).delete_connector(connector_name)


@when('we try to pause connector "{connector_name}"')
@step_require('cid')
def step_pause_connector(context, connector_name):
    get_kafka_cluster(context).pause_connector(connector_name)


@when('we try to resume connector "{connector_name}"')
@step_require('cid')
def step_resume_connector(context, connector_name):
    get_kafka_cluster(context).resume_connector(connector_name)


@when('we try to update connector')
@step_require('cid')
def step_update_connector(context):
    get_kafka_cluster(context).update_connector()
