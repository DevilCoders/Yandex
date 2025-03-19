"""
Provides general helpers for behave step execution.
"""

import functools
import json
from functools import wraps

import requests
import yaml
from behave.runner import Context
from google.protobuf import field_mask_pb2
from tests.helpers.grpcutil.exceptions import GRPCError
from tests.helpers.workarounds import retry

from tests.helpers.utils import context_to_dict, render_template, merge, ssh_checked
from .hadoop_cluster import get_default_dataproc_version, get_default_dataproc_version_prefix


def step_require(*vars_required):
    """
    Helper to enforce required context attributes
    before calling the body
    """

    def decorator(step_fun):
        @wraps(step_fun)
        def step_wrapper(context, *args, **kwargs):
            assert isinstance(context, Context), 'first argument for {0} must be `context`'.format(step_fun.__name__)
            for expected in vars_required:
                assert expected in context, '{0} should be set before calling {1}'.format(expected, step_fun.__name__)
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
    body = req.body
    if isinstance(body, bytes):
        body = body.decode('utf-8')
    data = "-d '{}'".format(body) if body else ''  # noqa
    headers = ['"{0}: {1}"'.format(k, v) for k, v in req.headers.items() if k not in CURL_SKIP_HEADERS]
    return command.format(method=method, headers=' -H '.join(headers), data=data, uri=uri)


def store_grpc_exception(step_func):
    """
    Stores exception of type GRPCError in context.grpc_exception
    """

    @wraps(step_func)
    def wrapper(context, *args, **kwargs):
        try:
            return step_func(context, *args, **kwargs)
        except GRPCError as e:
            setattr(context, 'grpc_exception', e)

    return wrapper


def print_request_on_fail(step_func):
    """
    Print curl command to stdout
    """

    @wraps(step_func)
    def wrapper(context, *args, **kwargs):
        try:
            return step_func(context, *args, **kwargs)
        except AssertionError:
            if hasattr(context, 'response'):
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


@retry(wait_fixed=20, stop_max_attempt_number=6)
def get_controlplane_time(context):
    """
    Returns time on control-plane VM.
    In most cases we can just use time.time().
    But in PITR scenarios time drift between control-plane and test machine
    may lead to errors like 'PITR time is in the future' and 'cluster not found'
    """
    out = ssh_checked(context.conf['compute_driver']['fqdn'], ['date', '+%s'], timeout=20)
    return int(out)


def store_time(context, time_key):
    """
    Store current time in the context.
    """
    if 'remembered_times' not in context.state:
        context.state['remembered_times'] = {}
    context.state['remembered_times'][time_key] = get_controlplane_time(context)


def get_time(context, time_key):
    """
    Get previously stored time from the context.
    """
    return context.state['remembered_times'][time_key]


def get_step_data(context):
    """
    Return step data deserialized from YAML or JSON representation and processed by template engine.
    """
    if context.text:
        # make this function is availble in jinja templates with preset context argument
        context.get_default_dataproc_version = functools.partial(get_default_dataproc_version, context)
        context.get_default_dataproc_version_prefix = functools.partial(get_default_dataproc_version_prefix, context)
        rendered_text = render_template(context.text, context_to_dict(context))
        if context.text.lstrip().startswith('{'):
            return json.loads(rendered_text)
        else:
            return yaml.safe_load(rendered_text)
    return {}


def render_text(context, template):
    """
    Return rendered template
    """
    return render_template(template, context_to_dict(context))


def fill_update_mask(request, update_dict):
    """
    Fills update_mask field of update request
    """
    if 'update_mask' not in update_dict:
        update_mask_items = build_update_mask(update_dict)
        request.update_mask.CopyFrom(field_mask_pb2.FieldMask(paths=update_mask_items))


def build_update_mask(dictionary, prefix=''):
    """
    Returns list of field paths mentioned within dictionary
    """
    res = []
    for key, val in dictionary.items():
        key_with_prefix = f'{prefix}{key}'

        if isinstance(val, dict):
            res.extend(build_update_mask(val, f'{key_with_prefix}.'))
        else:
            res.append(key_with_prefix)
    return res


def apply_overrides_to_cluster_config(cluster_config, config_override):
    """
    Apply overrides to cluster config, take into account some usability shortcuts
    """
    shortcuts = config_override.pop('shortcuts', None)
    cluster_config = merge(cluster_config, config_override)

    if shortcuts:
        for subcluster in cluster_config['configSpec']['subclustersSpec']:
            if subcluster['role'] == 'DATANODE':
                if shortcuts.get('datanodes_count'):
                    subcluster['hostsCount'] = shortcuts['datanodes_count']
                if shortcuts.get('datanodes_preset'):
                    subcluster['resources']['resourcePresetId'] = shortcuts['datanodes_preset']
            if subcluster['role'] == 'MASTERNODE':
                if shortcuts.get('masternodes_preset'):
                    subcluster['resources']['resourcePresetId'] = shortcuts['masternodes_preset']
            if shortcuts.get('subnet_id'):
                subcluster['subnetId'] = shortcuts['subnet_id']

        if shortcuts.get('bucket'):
            cluster_config['bucket'] = shortcuts['bucket']

        services = shortcuts.get('services')
        if services:
            if type(services) == str:
                services = [s.strip() for s in services.split(',')]
            cluster_config['configSpec']['hadoop']['services'] = services

        if shortcuts.get('properties'):
            cluster_config['configSpec']['hadoop']['properties'] = shortcuts['properties']

        if shortcuts.get('service_account_id'):
            cluster_config['configSpec']['hadoop']['properties'] = shortcuts['service_account_id']

    return cluster_config
