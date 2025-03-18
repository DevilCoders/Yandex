# coding=utf-8

import pytest
import requests

from antiadblock.cryprox.cryprox.common.cryptobody import crypt_url
from antiadblock.cryprox.cryprox.config import system as system_config
from antiadblock.cryprox.tests.lib.containers_context import TEST_LOCAL_HOST
from antiadblock.cryprox.tests.lib.an_yandex_utils import expand_an_yandex


@pytest.mark.parametrize('method, result', [('get', 200), ('post', 200)] + [(method, 405) for method in
                                                                            ('head', 'patch', 'put', 'delete', 'options')])
@pytest.mark.usefixtures('stub_server')
def test_supported_http_methods(method, result, get_key_and_binurlprefix_from_config, get_config):
    test_config = get_config('test_local')
    tested_method = getattr(requests, method)
    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)
    crypted_link = crypt_url(binurlprefix, 'http://{}/test.bin'.format(TEST_LOCAL_HOST), key, True, origin='test.local')
    response = tested_method(crypted_link,
                             headers={"host": TEST_LOCAL_HOST, system_config.PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]}, data={})
    assert response.headers.get(system_config.NGINX_SERVICE_ID_HEADER) == 'test_local'
    assert result == response.status_code


@pytest.mark.parametrize('host,code', []
    + expand_an_yandex('/an', 'an.yandex.ru', 405)
    + expand_an_yandex('/an', 'an.yandex.ru/count', 200)
    + expand_an_yandex('/an', 'an.yandex.ru/tracking', 200)
    + expand_an_yandex('/ads', 'an.yandex.ru/meta', 405)
    + [
    ('test.local', 200),
    ('ads.adfox.ru', 405),
    ('yastatic.net', 405),
])
def test_supported_post_requests(stub_server, host, code, get_config, get_key_and_binurlprefix_from_config):
    """
    POST запросы разрешены только на partner или count урлы
    """
    def handler(**_):
        return {'text': '', 'code': 200,
                'headers': {'Content-Type': 'application/javascript'}}
    stub_server.set_handler(handler)

    test_config = get_config('test_local')
    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)
    crypted_link = crypt_url(binurlprefix, 'http://{}/script.js'.format(host), key, True, origin='test.local')
    response = requests.post(crypted_link,
                             headers={"host": TEST_LOCAL_HOST, system_config.PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]}, data={})
    assert response.headers.get(system_config.NGINX_SERVICE_ID_HEADER) == 'test_local'
    assert code == response.status_code
