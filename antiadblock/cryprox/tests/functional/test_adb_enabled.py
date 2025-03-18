import pytest
import requests

from urlparse import urljoin, parse_qs

from antiadblock.cryprox.cryprox.common.cryptobody import crypt_url
from antiadblock.cryprox.cryprox.config import system as system_config
from antiadblock.cryprox.tests.lib.containers_context import TEST_LOCAL_HOST, AUTO_RU_HOST
from antiadblock.cryprox.tests.lib.an_yandex_utils import expand_an_yandex


def handler_adb_enabled(**request):
    query = parse_qs(request.get('query', ''))
    if query.get('adb_enabled', [''])[0] == '1':
        return {'text': 'ok', 'code': 200}
    else:
        return {'text': 'fail', 'code': 400}


@pytest.mark.parametrize('args,expected_code,expected_text', [
    ('', 400, 'fail'),
    ('adb_enabled=123', 400, 'fail'),
    ('adb_enabled=1', 200, 'ok')])
def test_nginx_test_adb_enabled(stub_server, args, expected_code, expected_text):
    stub_server.set_handler(handler_adb_enabled)

    response = requests.get(urljoin(stub_server.url, '/adb_enabled?' + args), headers={'host': 'test.local'})

    assert response.status_code == expected_code
    assert response.text == expected_text


def handler_no_adb_enabled(**request):
    if 'adb_enabled' not in request.get('query', ''):
        return {'text': 'ok', 'code': 200}
    else:
        return {'text': 'fail', 'code': 400}


@pytest.mark.parametrize('args,expected_code,expected_text', [
    ('', 200, 'ok'),
    ('adb_enabled', 400, 'fail'),
    ('adb_enabled=123', 400, 'fail'),
    ('adb_enabled=1', 400, 'fail')])
def test_nginx_test_no_adb_enabled(stub_server, args, expected_code, expected_text):
    stub_server.set_handler(handler_no_adb_enabled)

    response = requests.get(urljoin(stub_server.url, '/no_adb_enabled?' + args), headers={'host': 'test.local'})

    assert response.status_code == expected_code
    assert response.text == expected_text


@pytest.mark.parametrize('url', ['http://an.yandex.ru/meta/adb_enabled',
                                 'http://yandex.ru/ads/meta/adb_enabled',
                                 'http://ads.adfox.ru/123/getCode/adb_enabled',
                                 'http://yabs.yandex.ru/meta/adb_enabled',
                                 'http://yabs.yandex.by/meta/adb_enabled'])
@pytest.mark.parametrize('partner_name', ["test_local", 'autoru'])
def test_proxy_test_adb_enabled(stub_server, partner_name, url, get_key_and_binurlprefix_from_config, get_config):
    """
    Test cryprox: expect adb_enabled=1 param in rtb urls
    """
    stub_server.set_handler(handler_adb_enabled)

    key, binurlprefix = get_key_and_binurlprefix_from_config(partner_name)

    crypted_link = crypt_url(binurlprefix, url, key, True)
    response = requests.get(crypted_link, headers={'host': TEST_LOCAL_HOST, system_config.PARTNER_TOKEN_HEADER_NAME: get_config(partner_name).PARTNER_TOKENS[0]})

    assert 'ok' == response.text


@pytest.mark.parametrize('url', ['http://an.yandex.ru/meta/no_adb_enabled',
                                 'http://yandex.ru/ads/meta/no_adb_enabled',
                                 'http://yabs.yandex.ru/meta/no_adb_enabled',
                                 'http://yabs.yandex.by/meta/no_adb_enabled',
                                 'http://ads.adfox.ru/123/getCode/no_adb_enabled'])
def test_proxy_test_no_adb_enabled(stub_server, url, get_key_and_binurlprefix_from_config, get_config):
    """
    Test cryprox: no expect adb_enabled=1 param for non-rtb urls
    """
    stub_server.set_handler(handler_no_adb_enabled)
    key, binurlprefix = get_key_and_binurlprefix_from_config('yandex_morda')

    crypted_link = crypt_url(binurlprefix, url, key, True)
    response = requests.get(crypted_link, headers={'host': AUTO_RU_HOST, system_config.PARTNER_TOKEN_HEADER_NAME: get_config('autoru').PARTNER_TOKENS[0]})

    assert 'ok' == response.text


@pytest.mark.parametrize('url,disable_adb_enabled,expected_adb_enabled', []
    + expand_an_yandex('/an', 'http://an.yandex.ru/count/ololo', True, True)
    + expand_an_yandex('/an', 'http://an.yandex.ru/count/ololo', False, True)
    + [
    ('http://bs.yandex.ru/count/ololo', True, True),
    ('http://bs.yandex.ru/count/ololo', False, True),
    ('http://yabs.yandex.ru/count/ololo', True, False),
    ('http://yabs.yandex.by/count/ololo', True, False),
    ('http://yabs.yandex.ru/count/ololo', False, True),
    ('http://yabs.yandex.by/count/ololo', False, True),
    ('http://yabs.yandex.ru/meta/ololo', True, True),
    ('http://yabs.yandex.by/meta/ololo', True, True),
])
def test_disable_adb_enabled(stub_server, disable_adb_enabled, expected_adb_enabled, url, get_key_and_binurlprefix_from_config, get_config, set_handler_with_config):
    if expected_adb_enabled:
        stub_server.set_handler(handler_adb_enabled)
    else:
        stub_server.set_handler(handler_no_adb_enabled)

    partner_name = "test_local"
    test_config = get_config(partner_name)
    new_test_config = test_config.to_dict()
    new_test_config['DISABLE_ADB_ENABLED'] = disable_adb_enabled

    set_handler_with_config(test_config.name, new_test_config)

    key, binurlprefix = get_key_and_binurlprefix_from_config(partner_name)

    crypted_link = crypt_url(binurlprefix, url, key, True)
    response = requests.get(crypted_link, headers={'host': TEST_LOCAL_HOST, system_config.PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]})

    assert 'ok' == response.text
