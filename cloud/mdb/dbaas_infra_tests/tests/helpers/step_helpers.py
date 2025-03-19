"""
Provides general helpers for behave step execution.
"""
import datetime
from functools import wraps

import requests
import yaml
from behave.runner import Context

from tests.helpers.utils import context_to_dict, render_template


def step_require(*vars_required):
    """
    Helper to enforce required context attributes
    before calling the body
    """

    def decorator(step_fun):
        @wraps(step_fun)
        def step_wrapper(context, *args, **kwargs):
            assert isinstance(context, Context), \
                'first argument for {0} must be `context`'.format(
                    step_fun.__name__)
            for expected in vars_required:
                assert expected in context, \
                    '{0} should be set before calling {1}'.format(
                        expected, step_fun.__name__)
            return step_fun(context, *args, **kwargs)

        return step_wrapper

    return decorator


CURL_SKIP_HEADERS = {
    'Connection',
    'Accept-Encoding',
    'User-Agent',
    'Content-Length',
}


def make_curl(req: requests.Request) -> str:
    """
    Construct curl command from requests.Request
    """
    command = "curl -k -X {method} -H {headers} {data} '{uri}'"
    method = req.method
    uri = req.url
    data = "-d '%s'" % req.body if req.body else ''  # noqa
    headers = ['"{0}: {1}"'.format(k, v) for k, v in req.headers.items() if k not in CURL_SKIP_HEADERS]
    return command.format(method=method, headers=' -H '.join(headers), data=data, uri=uri)


def print_request_on_fail(step_func):
    """
    Print curl command to stdout
    """

    @wraps(step_func)
    def wrapper(context, *args, **kwargs):
        try:
            return step_func(context, *args, **kwargs)
        except AssertionError:
            print(make_curl(context.response.request))
            raise

    return wrapper


def store_response(context, response_key):
    """
    Store the response in the context.
    """
    if 'remembered_responses' not in context.state:
        context.state['remembered_responses'] = {}
    context.state['remembered_responses'][response_key] = context.response


def get_response(context, response_key):
    """
    Get previously stored response from the context.
    """
    return context.state['remembered_responses'][response_key]


def store_timestamp(context, ts_key):
    """
    Store current timestamp in the context.
    """
    if 'remembered_timestamps' not in context.state:
        context.state['remembered_timestamps'] = {}
    context.state['remembered_timestamps'][ts_key] = datetime.datetime.now(datetime.timezone.utc)


def store_timestamp_value(context, ts_key, ts_value):
    """
    Store given timestamp in the context.
    """
    if 'remembered_timestamps' not in context.state:
        context.state['remembered_timestamps'] = {}
    context.state['remembered_timestamps'][ts_key] = ts_value


def get_timestamp(context, ts_key):
    """
    Get previously stored timestamp from the context.
    """
    return context.state['remembered_timestamps'][ts_key]


def get_step_data(context):
    """
    Return step data deserialized from YAML representation and processed by template engine.
    """
    return yaml.load(render_template(context.text, context_to_dict(context))) if context.text else {}
