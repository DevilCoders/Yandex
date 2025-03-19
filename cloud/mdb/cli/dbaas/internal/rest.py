"""
Utilities for dealing with REST services.
"""

import json
import sys
from json import JSONDecodeError

import requests
from click import ClickException

from cloud.mdb.cli.common.formatting import print_response
from cloud.mdb.cli.dbaas.internal.config import get_config, get_environment_name
from cloud.mdb.cli.dbaas.internal.iam import get_iam_token
from cloud.mdb.cli.dbaas.internal.utils import debug_mode, dry_run_mode, get_oauth_token


def rest_request(ctx, config_section, http_method, url_path, data=None, suppress_errors=False, token=None, **kwargs):
    config = get_config(ctx).get(config_section, {})

    rest_endpoint = config.get('rest_endpoint')
    if not rest_endpoint:
        raise ClickException(f'Service "{config_section}" is not available in "{get_environment_name(ctx)}".')

    session = _rest_session(ctx, config_section, token=token)

    url = rest_endpoint + url_path
    verify = config.get('ca_path', True)

    if debug_mode(ctx):
        message = f'{http_method.upper()} {url}\n'
        if data:
            message += f'{json.dumps(data, indent=2, ensure_ascii=False)}\n'
        # print(f'{http_method.upper()} {url}', file=sys.stderr)
        # if req.body:
        #     print(req.body.decode(), file=sys.stderr)
        print(message, file=sys.stderr)

    if dry_run_mode(ctx):
        return None

    response = session.request(http_method.lower(), url, verify=verify, json=data, **kwargs)

    try:
        result = response.json()
    except JSONDecodeError:
        result = response.text or None

    if response.status_code >= 400 and not suppress_errors:
        print_response(ctx, result)
        exit(1)

    return result


def _rest_session(ctx, config_section, token=None):
    ctx_key = f'{config_section}_session'
    if ctx_key not in ctx.obj:
        config = ctx.obj['config'][config_section]

        requests.packages.urllib3.disable_warnings()
        session = requests.session()
        session.headers = {
            'Accept': 'application/json',
            'Content-Type': 'application/json',
        }
        auth = config.get('rest_authorization')
        if auth == 'yacloud_subject_token':
            if token is None:
                token = get_iam_token(ctx)
            session.headers['X-YaCloud-SubjectToken'] = token
        elif auth == 'oauth':
            if token is None:
                token = get_oauth_token(ctx, config_section)
            session.headers['Authorization'] = f'OAuth {token}'

        ctx.obj[ctx_key] = session

    return ctx.obj[ctx_key]
