"""
Steps related to more than one entity
"""
import time

import humanfriendly
from behave import then, when
from hamcrest import (all_of, assert_that, equal_to, greater_than, has_entry, has_length, instance_of)

from tests.helpers.step_helpers import (get_response, print_request_on_fail, step_require, store_response,
                                        store_timestamp)


@then('response should have status {value}')
@then('response should have status {value} and body contains "{text}"')
@then('response should have status {value} and body equals')
@step_require('response')
@print_request_on_fail
def step_check_response(context, value, text=''):
    actual = context.response.status_code
    if not text:
        text = context.text
    assert str(actual) == str(value), ('Got response with unexpected status ({actual}), expected: {value}'.format(
        actual=actual, value=value))
    assert not text or (text in context.response.text), ('Unexpected body={body}'.format(body=context.response.text))


def is_list_at_key_with_length_greater(key: str, length: int):
    """
    Combo length_greater matcher
    """
    return has_entry(key, all_of(has_length(greater_than(length)), instance_of(list)))


def is_list_at_key_with_length_equal(key: str, length: int):
    """
    Combo length_equal matcher
    """
    return has_entry(key, all_of(has_length(equal_to(length)), instance_of(list)))


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


@then('message with "{key:w}" list is larger than in "{response_key:w}"')
@step_require('response')
@print_request_on_fail
def step_check_response_has_greater_size(context, key, response_key):
    remembered_length = len(response_data(get_response(context, response_key))[key])
    assert_that(response_data(context.response), is_list_at_key_with_length_greater(key, remembered_length))


def response_data(resp):
    """
    extract data from response (python api or go api)
    """
    # for go api we usually return dict
    if isinstance(resp, dict):
        return resp
    # for python api we usually return requests.Response object
    return resp.json()


@then('message with "{key:w}" list length is equal to number "{length:d}"')
@then('message should have empty "{key:w}" list')
@step_require('response')
@print_request_on_fail
def step_check_response_has_equal_number_size(context, key, length=0):
    assert_that(response_data(context.response), is_list_at_key_with_length_equal(key, length))


@then('message with "{key:w}" list length is equal to "{response_key:w}"')
@step_require('response')
@print_request_on_fail
def step_check_response_has_equal_size(context, key, response_key):
    remembered_length = len(response_data(get_response(context, response_key))[key])
    assert_that(response_data(context.response), is_list_at_key_with_length_equal(key, remembered_length))


@then('we wait for "{timeout}"')
def step_nap(_, timeout):
    time.sleep(humanfriendly.parse_timespan(timeout))


@when('we remember current timestamp as "{ts_key:w}"')
@then('we remember current timestamp as "{ts_key:w}"')
def step_remember_timestamp(context, ts_key):
    store_timestamp(context, ts_key)
