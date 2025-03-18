# -*- coding: utf8 -*-
from urlparse import urlparse

import pytest

from antiadblock.cryprox.cryprox.common.tools import url as url_tool
from antiadblock.cryprox.cryprox.common.tools.misc import update_dict, parse_nonce_from_headers
from antiadblock.cryprox.cryprox.common.tools.js_minify import Minify


@pytest.mark.parametrize('query_string, param, arg, overwrite, expected_query_string',
                         [
                             ('', 'ext-uid-id', '12345', False, 'ext-uid-id=12345'),
                             ('adblock=1&tag=test_extuid_tag', 'ext-uid-id', '12345', False, 'adblock=1&tag=test_extuid_tag&ext-uid-id=12345'),
                             ('adblock=1&tag=test_extuid_tag', 'ext-uid-id', '12345', True, 'adblock=1&tag=test_extuid_tag&ext-uid-id=12345'),
                             ('adblock=1&ext-uid-id=000', 'ext-uid-id', '12345', False, 'adblock=1&ext-uid-id=000&ext-uid-id=12345'),
                             ('adblock=1&ext-uid-id=000', 'ext-uid-id', '12345', True, 'adblock=1&ext-uid-id=12345'),
                             ('adblock=1&ext-uid-id=000&ext-uid-id=999', 'ext-uid-id', '12345', True, 'adblock=1&ext-uid-id=12345'),
                             ('adblock=✓&tag=test', 'ext-uid-id', '12345', False, 'adblock=%E2%9C%93&tag=test&ext-uid-id=12345'),
                         ])
def test_url_add_query_param(query_string, param, arg, overwrite, expected_query_string):

    url = urlparse('http://test.local/test?' + query_string)
    expected_url = url._replace(query=expected_query_string)
    assert url_tool.url_add_query_param(url, param, arg, overwrite) == expected_url


@pytest.mark.parametrize('original_query_string, params_to_delete, expected_query_string',
                         [
                             ('flash=true&barry=allen', ['flask', 'allen'], 'flash=true&barry=allen'),
                             ('flash=true&barry=allen', ['flash', 'allen'], 'barry=allen'),
                             ('flash=true&barry=allen', ['flash', 'barry'], ''),
                             ('flash=true&barry=allen', [], 'flash=true&barry=allen'),
                             ('', ['flask', 'allen'], ''),
                         ])
def test_url_delete_query_params(original_query_string, params_to_delete, expected_query_string):

    url = urlparse('http://test.local/test?' + original_query_string)
    expected_url = url._replace(query=expected_query_string)
    assert url_tool.url_delete_query_params(url, params_to_delete) == expected_url


@pytest.mark.parametrize('base, merge, expected', [
    (dict(a=1), dict(b=2), dict(a=1, b=2)),
    (dict(a=1, b=2), dict(b=3), dict(a=1, b=2)),
    (dict(a=1, b=dict(c=3, d=4)), dict(b=dict(d=5, e=6)), dict(a=1, b=dict(c=3, d=4, e=6))),
])
def test_update_dict(base, merge, expected):
    assert update_dict(base, merge) == expected


@pytest.mark.parametrize('headers, expected', [
    ({}, ('', '')),
    ({'Content-Type': 'utf-8'}, ('', '')),
    ({'Content-Security-Policy': "default-src 'nonce-2726c7f26c';"}, ('2726c7f26c', '2726c7f26c')),
    ({'Content-Security-Policy': "default-src 'self';"}, ('', '')),
    ({'Content-security-policy': "script-src 'nonce-2726c7f26c';"}, ('2726c7f26c', '')),
    ({'Content-Security-Policy': "script-src 'self';"}, ('', '')),
    ({'CONTENT-Security-Policy': "default-src 'nonce-2726cxcv7f26c'; script-src 'nonce-2726c7f26c'; style-src 'nonce-ololo1style1tololo'; "}, ('2726c7f26c', 'ololo1style1tololo')),
    ({'Content-Security-Policy-Report-Only': "default-src 'nonce-2726c7f26c'; script-src 'self';"}, ('2726c7f26c', '2726c7f26c')),
    ({'Content-Security-Policy-Report-Only': "default-src 'nonce-2726c7f26c'; style-src 'self' 'nonce-blblblbc7f26c';"}, ('2726c7f26c', 'blblblbc7f26c')),
])
def test_parse_nonce_from_headers(headers, expected):
    assert parse_nonce_from_headers(headers) == expected


def test_js_minify():
    test_js = """function (url) { // inline comment
\tconst i = 11; //tabs and const\n
\tlet ololo = "trololo"; // let\n
/* многострочный еще и русский коммент!!!
*/
    if (url.indexOf('//') == 0) {  // не должно воспринять этот кусок текста как комментарий
        url_prefix.replace(/^https?:/, ''); // https://st.yandex-team.ru/ANTIADB-641
    }
        return url.indexOf("{url_prefix}") === 0 && url.indexOf("{seed}") !== -1;
}"""
    expected_result = '''function(url){var i=11;var ololo="trololo";if(url.indexOf('//')==0){url_prefix.replace(/^https?:/,'');}return url.indexOf("{url_prefix}")===0&&url.indexOf("{seed}")!==-1;}'''
    assert Minify(test_js).getMinify() == expected_result
