# -*- coding: utf8 -*-
import pytest
import requests

from antiadblock.cryprox.cryprox.common.cryptobody import crypt_url
from antiadblock.cryprox.cryprox.config import system as system_config

from antiadblock.cryprox.tests.lib.containers_context import TEST_LOCAL_HOST

DECRYPTED_COOKIE_VALUE = "6731871211515776902"
ENCRYPTED_COOKIE_VALUE = "euo9uyWw7d4iZjI08fRvZutgqjIcQMU3c4uHIgjAx6RlAUm+Y5No9Nts3stu5KxLxQIlvvVt0tfHF/27g8lK/ePOV5A="

TEST_CASES = [
    (
        {'crookie': ENCRYPTED_COOKIE_VALUE},
        {system_config.YAUID_COOKIE_NAME: DECRYPTED_COOKIE_VALUE},
    ),
    (
        {'crookie': 'bad_value'},
        {},
    ),
    (
        {system_config.YAUID_COOKIE_NAME: DECRYPTED_COOKIE_VALUE},
        {system_config.YAUID_COOKIE_NAME: DECRYPTED_COOKIE_VALUE},
    ),
    (
        {},
        {},
    ),
]


def handler(**request):
    headers = {}
    if system_config.YAUID_COOKIE_NAME in request['cookies']:
        headers.update({'cookie_' + system_config.YAUID_COOKIE_NAME: request['cookies'][system_config.YAUID_COOKIE_NAME]})
    return {'text': 'ok', 'code': 200, 'headers': headers}


@pytest.mark.parametrize('loader_url', (
    'http://an.yandex.ru/system/context_adb.js',
    'http://yandex.ru/ads/system/context_adb.js',
    'http://an.yandex.ru/system/adfox_adb.js',
    'http://yandex.ru/ads/system/adfox_adb.js',
    'http://an.yandex.ru/system/widget_adb.js',
    'http://yandex.ru/ads/system/widget_adb.js',
))
@pytest.mark.parametrize('request_cookies, expected_req_cookies', TEST_CASES)
def test_proxy_pcode_loader_url(stub_server, get_config, get_key_and_binurlprefix_from_config,
                           loader_url, request_cookies, expected_req_cookies):
    stub_server.set_handler(handler)
    test_config = get_config('test_local')
    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)
    crypted_link = crypt_url(binurlprefix, loader_url, key, True, origin='test.local')
    response = requests.get(crypted_link,
                            headers={'host': TEST_LOCAL_HOST, system_config.PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]},
                            cookies=request_cookies)
    assert response.status_code == 200
    for cookie, value in expected_req_cookies.items():
        assert response.headers.get('cookie_' + cookie, None) == value
