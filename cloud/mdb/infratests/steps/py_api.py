import logging
import time

import humanfriendly
from behave import given, then, when
from deepdiff import DeepDiff
from hamcrest import assert_that, equal_to, has_entries, has_item, has_items, has_key

from cloud.mdb.infratests.test_helpers import py_api
from cloud.mdb.infratests.test_helpers.context import Context
from cloud.mdb.infratests.test_helpers.utils import get_step_data


@then('response should have status {status:d}')
@then('response should have status {status:d} and body contains')
@then('response should have status {status:d} and key {key:w} list contains')
def step_check_py_api_response(context: Context, status: int, key: str = None):
    actual = context.response.status_code
    assert str(actual) == str(
        status
    ), 'Got response with unexpected status ({actual}), ' 'expected: {value}, [body={body}]'.format(
        actual=actual, value=status, body=context.response.text
    )
    if context.text:
        expected_response_body = get_step_data(context)
        actual_response_body = context.response.json()
        if key:
            actual_response_body = actual_response_body[key]
            if isinstance(expected_response_body, list):
                assert_that(
                    actual_response_body,
                    has_items(*expected_response_body),
                    'Unexpected diff: {diff}'.format(diff=DeepDiff(expected_response_body, actual_response_body)),
                )
            else:
                assert_that(
                    actual_response_body,
                    has_item(expected_response_body),
                    'Unexpected diff: {diff}'.format(diff=DeepDiff(expected_response_body, actual_response_body)),
                )
        else:
            assert_that(
                actual_response_body,
                has_entries(expected_response_body),
                'Unexpected diff: {diff}'.format(diff=DeepDiff(expected_response_body, actual_response_body)),
            )


@given('cluster "{cluster_name}" is up and running')
@then('cluster "{cluster_name}" is up and running')
def step_cluster_is_running(context: Context, cluster_name: str):
    py_api.load_cluster_into_context(context, cluster_name)
    cluster = context.cluster
    assert cluster['status'] == 'RUNNING', f"Cluster has unexpected status {cluster['status']}"
    assert cluster['health'] == 'ALIVE', f"Cluster has unexpected health {cluster['health']}"


@when('we attempt to modify cluster "{cluster_name}" with following parameters')
def step_modify_cluster(context: Context, cluster_name: str = None):
    context.response = py_api.modify_cluster(context, get_step_data(context))
    context.operation_id = context.response.json().get('id')


@then('we wait no more than "{timeout}" until cluster health is {health}')
def step_wait_cluster_health(context: Context, timeout: str, health: str):
    deadline = time.time() + humanfriendly.parse_timespan(timeout)
    equals = False
    real_health = ""
    while time.time() < deadline and not equals:
        cluster = py_api.get_cluster(context, context.cluster['name'], context.test_config.folder_id)
        real_health = cluster['health']
        equals = real_health == health
    if not equals:
        assert_that(
            real_health, equal_to(health), 'Timed out waiting until cluster health will become equal to expected'
        )


@then('generated task is finished within "{timeout}"')
def step_wait_task_is_finished(context: Context, timeout: str):
    deadline = time.time() + humanfriendly.parse_timespan(timeout)
    finished_successfully = False
    error = "Task not finished until timeout"

    while time.time() < deadline:
        task = py_api.get_task(context)
        assert_that(task, has_key('done'))

        if not task['done']:
            time.sleep(1)
            continue

        logging.debug('Task is done: %r', task)

        finished_successfully = 'error' not in task
        if not finished_successfully:
            error = task['error']
        break

    assert finished_successfully, '{operation_id} error is {error}'.format(
        operation_id=context.operation_id, error=error
    )
