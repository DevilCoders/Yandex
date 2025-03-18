# -*- coding: utf8 -*-
import pytest
import json
import requests
from urllib import quote
from werkzeug.wrappers import Response
from hamcrest import assert_that, has_entries

from antiadblock.cryprox.cryprox.common.cryptobody import crypt_url, generate_hide_meta_args_header_name
from antiadblock.cryprox.cryprox.config import system as system_config
from antiadblock.cryprox.tests.lib.containers_context import TEST_LOCAL_HOST
from antiadblock.cryprox.tests.lib.an_yandex_utils import expand_an_yandex

HIDDEN_GRAB_HEADER_NAME = generate_hide_meta_args_header_name("my2007", system_config.HIDE_GRAB_HEADER_RE)
HIDDEN_URI_PATH_HEADER_NAME = generate_hide_meta_args_header_name("my2007", system_config.HIDE_URI_PATH_HEADER_RE)


@pytest.mark.parametrize('url, uri_path_in_headers, grab_in_headers, expected_uri_path', [
    ('http://yastatic.net/', 'path/file', '', '/path/file'),
    ('http://yastatic.net/', '/path/file', '', '/path/file'),
    ('http://yastatic.net/pa', 'th/file', '', '/path/file'),
    ('http://yastatic.net/path/', 'file', '', '/path/file'),
    ('http://yastatic.net/path/file', '', '', '/path/file'),
    ('http://yastatic.net/', 'path/file?arg1=val1&arg2=val2', '', '/path/file?arg1=val1&arg2=val2'),
    ('http://yastatic.net/pa', 'th/file?arg1=val1&arg2=val2', '', '/path/file?arg1=val1&arg2=val2'),
    ('http://yastatic.net/path/', 'file?arg1=val1&arg2=val2', '', '/path/file?arg1=val1&arg2=val2'),
    ('http://yastatic.net/path/file', '?arg1=val1&arg2=val2', '', '/path/file?arg1=val1&arg2=val2'),
    ('http://yastatic.net/path/file?arg1=val1', '&arg2=val2', '', '/path/file?arg1=val1&arg2=val2'),
    ('http://yastatic.net/path/', 'file', 'some_grab_value', '/path/file?grab=some_grab_value'),
    ('http://yastatic.net/path/file?arg1=val1', '&arg2=val2', 'some_grab_value', '/path/file?arg1=val1&arg2=val2&grab=some_grab_value'),
    ('http://yastatic.net/path/file', '', 'some_grab_value', '/path/file'),
])
def test_hidden_uri_path_header(url, uri_path_in_headers, expected_uri_path, stub_server, get_config, get_key_and_binurlprefix_from_config, grab_in_headers):
    def handler(**request):
        for header_name in (HIDDEN_URI_PATH_HEADER_NAME, HIDDEN_GRAB_HEADER_NAME):
            if ('HTTP_' + header_name).upper() in request.get("headers"):
                return Response("hidden header should be removed after proxing", status=500)
        query = request.get('query', '')
        if query:
            query = '?' + query
        return Response(request.get('path', '') + query)

    stub_server.set_handler(handler)
    test_config = get_config('test_local')
    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)
    crypted_link = crypt_url(binurlprefix, url, key, True)
    request_headers = {'host': TEST_LOCAL_HOST,
                       system_config.PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]}
    if uri_path_in_headers:
        request_headers[HIDDEN_URI_PATH_HEADER_NAME] = uri_path_in_headers
    if grab_in_headers:
        request_headers[HIDDEN_GRAB_HEADER_NAME] = grab_in_headers
    response = requests.get(crypted_link, headers=request_headers)
    assert response.status_code == 200
    assert expected_uri_path == response.text
