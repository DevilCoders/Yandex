"""
Steps related to more than one entity
"""
import json
import random
import string
import time

import humanfriendly
from behave import given, then, when
from deepdiff import DeepDiff
from google.protobuf import json_format
from hamcrest import (
    all_of,
    assert_that,
    equal_to,
    greater_than,
    has_entries,
    has_entry,
    has_length,
    instance_of,
    has_item,
    has_items,
    contains_string,
    has_property,
)
from tests.helpers.grpcutil.exceptions import GRPCError

from tests.helpers.pillar import mdb_internal_api_pillar
from tests.helpers.s3 import create_bucket
from tests.helpers.step_helpers import (
    get_response,
    get_step_data,
    print_request_on_fail,
    step_require,
    store_response,
    store_time,
)
from tests.helpers.utils import context_to_dict, render_template


@then('grpc response should have status {status}')
@step_require('response')
@print_request_on_fail
def step_check_grpc_response(context, status):
    actual_response_body = json_format.MessageToDict(context.response)
    actual = actual_response_body['response']['message']
    if 'response' not in actual_response_body:
        raise Exception('Got and unexpected GRPC response', actual_response_body)

    if context.text:
        rendered_expected_content = render_template(context.text, context_to_dict(context))
        expected_in_response_body = json.loads(rendered_expected_content)
        assert_that(
            actual_response_body,
            has_entries(expected_in_response_body),
            'Unexpected diff: {diff}'.format(diff=DeepDiff(expected_in_response_body, actual_response_body)),
        )

    assert str(status) in str(
        actual
    ), 'Got response with unexpected status ({actual}), ' 'expected: {value}, [body={body}]'.format(
        actual=actual, value=status, body=actual_response_body
    )


@then('grpc response should fail with status {status} and body contains')
@step_require('grpc_exception')
@print_request_on_fail
def step_check_grpc_fail(context, status):
    assert_that(context, has_property('grpc_exception'))
    assert_that(context.grpc_exception, instance_of(GRPCError))

    grpc_error = context.grpc_exception
    actual = grpc_error.err_type
    actual_response_body = grpc_error.message

    if context.text:
        assert_that(
            actual_response_body,
            contains_string(context.text),
            'Unexpected diff: {diff}'.format(diff=DeepDiff(context.text, actual_response_body)),
        )

    assert str(status) == str(
        actual
    ), 'Got response with unexpected status ({actual}), ' 'expected: {value}, [body={body}]'.format(
        actual=actual, value=status, body=actual_response_body
    )


@then('response should have status {status:d}')
@then('response should have status {status:d} and body contains')
@then('response should have status {status:d} and key {key:w} list contains')
@step_require('response')
@print_request_on_fail
def step_check_response(context, status, key=None):
    actual = context.response.status_code
    assert str(actual) == str(
        status
    ), 'Got response with unexpected status ({actual}), ' 'expected: {value}, [body={body}]'.format(
        actual=actual, value=status, body=context.response.text
    )
    if context.text:
        rendered_expected_content = render_template(context.text, context_to_dict(context))
        expected_in_response_body = json.loads(rendered_expected_content)
        actual_response_body = context.response.json()
        if key:
            actual_response_body = actual_response_body[key]
            if isinstance(expected_in_response_body, list):
                assert_that(
                    actual_response_body,
                    has_items(*expected_in_response_body),
                    'Unexpected diff: {diff}'.format(diff=DeepDiff(expected_in_response_body, actual_response_body)),
                )
            else:
                assert_that(
                    actual_response_body,
                    has_item(expected_in_response_body),
                    'Unexpected diff: {diff}'.format(diff=DeepDiff(expected_in_response_body, actual_response_body)),
                )
        else:
            assert_that(
                actual_response_body,
                has_entries(expected_in_response_body),
                'Unexpected diff: {diff}'.format(diff=DeepDiff(expected_in_response_body, actual_response_body)),
            )


def is_list_at_key_with_length_greater(key: str, length: int):
    """
    Combo matcher
    """
    return has_entry(key, all_of(has_length(greater_than(length)), instance_of(list)))


@then('message with "{key:w}" list is larger than "{length:d}"')
@then('message should have not empty "{key:w}" list')
@step_require('response')
@print_request_on_fail
def step_check_response_message(context, key, length=0):
    assert_that(context.response.json(), is_list_at_key_with_length_greater(key, length))


@when('we remember current response as "{response_key:w}"')
@then('we remember current response as "{response_key:w}"')
@step_require('response')
def step_remember_response(context, response_key):
    store_response(context, response_key)


@when('we remember current time as "{time_key:w}"')
@then('we remember current time as "{time_key:w}"')
def step_remember_current_time(context, time_key):
    store_time(context, time_key)


@then('message with "{key:w}" list is larger than in "{response_key:w}"')
@step_require('response')
@print_request_on_fail
def step_check_response_has_greater_size(context, key, response_key):
    remembered_length = len(get_response(context, response_key).json()[key])
    assert_that(context.response.json(), is_list_at_key_with_length_greater(key, remembered_length))


@when('we wait for "{timeout}"')
@then('we wait for "{timeout}"')
def step_nap(_, timeout):
    time.sleep(humanfriendly.parse_timespan(timeout))


@given('put random string to context under {key:w} key')
def step_generate_random(context, key):
    letters = string.ascii_lowercase
    rnd = ''.join(random.choice(letters) for _ in range(20))
    setattr(context, key, rnd)


@given('put random s3 bucket name to context under {key:w} key')
def step_generate_random_s3_bucket_name(context, key):
    prefix = mdb_internal_api_pillar(context.conf)['config']['logic']['s3_bucket_prefix']
    letters = string.ascii_lowercase
    rnd = prefix + 'gen-' + ''.join(random.choice(letters) for _ in range(20))
    setattr(context, key, rnd)


@when('filter {var_name} list var with')
def step_filter_list_var(context, var_name):
    filters = get_step_data(context)
    lst = getattr(context, var_name)
    filtered = [obj for obj in lst if all(obj.get(key) == val for key, val in filters.items())]
    setattr(context, var_name, filtered)


@then('var {var_name} equals')
def step_check_var_equals(context, var_name):
    expected_value = get_step_data(context)
    value = getattr(context, var_name)
    assert_that(value, equal_to(expected_value))


@then('sleep for "{seconds:d}" seconds')
def sleep(context, seconds: int):
    time.sleep(seconds)


@given('s3 bucket with key {bucket_name_key} exists')
def s3_bucket_with_key_exists(context, bucket_name_key):
    bucket_name = str(getattr(context, bucket_name_key))
    create_bucket(context.conf['s3'], bucket_name)
