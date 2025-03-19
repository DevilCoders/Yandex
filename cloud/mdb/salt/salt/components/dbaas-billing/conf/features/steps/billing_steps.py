import os
import subprocess
import logging
import datetime
import time
import json

from behave import given, when, then
import dateutil.tz
from hamcrest import assert_that, equal_to, has_length, has_entries, matches_regexp

log = logging.getLogger(__name__)


@given('current time is "{ts:d}"')
@when('time ticks to "{ts:d}"')
def remember_time(context, ts):
    context.current_time = ts


def get_current_ts(context):
    if 'current_time' in context:
        return context.current_time
    return int(time.time())


def format_ts_to_str(ts):
    return datetime.datetime.fromtimestamp(ts, dateutil.tz.tzlocal())


@given('not existed billing state')
def step_init_billing_state(context):
    pass


@given('billing state')
def step_make_state_file(context):
    with open(get_state_file_path(context), 'w') as fd:
        fd.write(context.text)


@given('billing state not exists')
def step_make_state_file_not_exists(context):
    try:
        os.unlink(get_state_file_path(context))
    except OSError:
        pass


@given('billing context')
def step_parameters(context):
    context.parameters = json.loads(context.text)


@given('billing checks')
def step_checks(context):
    context.checks = json.loads(context.text)


def get_billing_file_path(context):
    return os.path.join(context.root, 'billing.log')


def get_service_file_path(context):
    return os.path.join(context.root, 'service.log')


def get_state_file_path(context):
    return os.path.join(context.root, 'billing.state')


@when('I call billing')
def step_call_billing(context):
    test_file = os.path.join(context.root, 'billing_test.py')
    with open(test_file, 'w') as fd:
        fd.write("""
import sys
import freezegun
import logging

sys.path.append('.')
import billing

billing.STATE_PATH = '{state_file}'
billing.MONRUN_CACHE_PATH = '{root}/monrun'
logging.basicConfig(
    level=logging.INFO
)

with freezegun.freeze_time('{current_time}'):
    billing.billing(
        log_file='{log}',
        service_log_file='{service_log}',
        rotate_size=0,
        params={params},
        checks={checks})

""".format(root=context.root,
           checks=repr(context.checks),
           params=repr(context.parameters),
           state_file=get_state_file_path(context),
           log=get_billing_file_path(context),
           service_log=get_service_file_path(context),
           current_time=format_ts_to_str(get_current_ts(context))))

    cmd = subprocess.Popen(['coverage', 'run', '--append', test_file],
                           stderr=subprocess.PIPE,
                           stdout=subprocess.PIPE)
    out, err = cmd.communicate()
    if out:
        log.info('Script out is %s', out)
    if err:
        log.info('Script err is %s', err)
    if cmd.returncode != 0:
        raise AssertionError('Fail with ' + err)


@then('billing log appears')
def step_billing_log_exists(context):
    assert os.path.exists(get_billing_file_path(context))


def get_billing_log(context):
    with open(get_billing_file_path(context)) as fd:
        return fd.read()


@then('billing log is empty')
def step_billing_log_is_empty(context):
    assert_that(get_billing_log(context), equal_to(''))


def split_lines(text):
    return [l for l in text.split('\n') if l]


@then('billing log contains "{count:d}" lines')
def step_billing_log_contains_records(context, count):
    lines = split_lines(get_billing_log(context))
    assert_that(lines, has_length(count))


def reformat_json(js_str):
    return json.dumps(json.loads(js_str), sort_keys=True)


def make_matcher_by_value(value):
    if isinstance(value, dict):
        kv_matches = {}
        for d_key, d_value in value.items():
            kv_matches[d_key] = make_matcher_by_value(d_value)
        return has_entries(**kv_matches)
    return equal_to(value)


@then('"{line_no:d}" line in billing log matches')
def step_billing_log_is(context, line_no):
    all_billing_lines = [l for l in get_billing_log(context).split('\n') if l]
    line = all_billing_lines[line_no]
    assert_that(
        json.loads(line), make_matcher_by_value(json.loads(context.text)))


def get_service_log(context):
    with open(get_service_file_path(context)) as fd:
        return fd.read()


@then('service log is empty')
def step_service_log_is_empty(context):
    assert_that(get_service_log(context), equal_to(''))


@then('service log is')
def step_service_log_is(context):
    assert_that(get_service_log(context).strip(), equal_to(context.text))


@then('service log matches')
def step_service_log_matches(context):
    assert_that(get_service_log(context).strip(), matches_regexp(context.text))
