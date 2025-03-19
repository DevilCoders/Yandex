import random
import string
import time

import humanfriendly
from behave import given, then, when
from hamcrest import assert_that, equal_to

from cloud.mdb.infratests.test_helpers.context import Context
from cloud.mdb.infratests.test_helpers.utils import get_step_data


@then('var {var_name} equals')
def step_check_var_equals(context: Context, var_name: str):
    expected_value = get_step_data(context)
    value = getattr(context, var_name)
    assert_that(value, equal_to(expected_value))


@when('filter {var_name} list var with')
def step_filter_list_var(context: Context, var_name: str):
    filters = get_step_data(context)
    lst = getattr(context, var_name)
    filtered = [obj for obj in lst if all(obj.get(key) == val for key, val in filters.items())]
    setattr(context, var_name, filtered)


@given('put random string to context under {key:w} key')
def step_put_random_string_to_context(context: Context, key: str):
    letters = string.ascii_lowercase
    rnd = ''.join(random.choice(letters) for _ in range(20))
    setattr(context, key, rnd)


@when('we wait for "{timeout}"')
@then('we wait for "{timeout}"')
def step_sleep(_, timeout):
    time.sleep(humanfriendly.parse_timespan(timeout))


@given('we are working with {cluster_type} cluster')
def step_working_with_cluster_type(context: Context, cluster_type: str):
    context.cluster_type = cluster_type
