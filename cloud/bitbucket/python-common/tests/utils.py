import os
import re
import tempfile

import responses

import yatest.common


def mock_response(method, url, status=200, response=None, mock_file=None):
    if mock_file:
        file = yatest.common.source_path(os.path.join('cloud/bitbucket/python-common/tests', mock_file))
        with open(file) as f:
            response = f.read()

    responses.add(method, url, body=response, status=status, content_type="application/json")


def mock_api(url, method, response=None, mock_file=None):
    def decorator(fn):
        def inner(*args, **kwargs):
            mock_response(method, url, response=response, mock_file=mock_file)
            return fn(*args, **kwargs)

        return inner

    return decorator


def mock_auth():
    mock_response(
        responses.POST,
        re.compile(r'.*/identity/public/v1/auth/oauth'),
        response='{"token": "TOKEN", "secret_key": "SECRET_KEY"}',
    )


def with_temp_config_file(fn):
    def wrapper(*args, **kwargs):
        with tempfile.NamedTemporaryFile() as f:
            fn(*args, **kwargs, config_file=f.name)

    return wrapper
