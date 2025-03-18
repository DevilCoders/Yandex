# coding=utf-8

import json
import requests
from urlparse import urljoin

import re2
import pytest

from library.python import resource

from antiadblock.cryprox.cryprox.config.ad_systems import AdSystem
from antiadblock.cryprox.tests.lib.config_stub import initial_config
from antiadblock.cryprox.cryprox.common.tools.regexp import re_expand
from antiadblock.cryprox.cryprox.config.bk import DisabledAdType
from antiadblock.cryprox.tests.lib.containers_context import TEST_LOCAL_HOST
from antiadblock.cryprox.cryprox.common.cryptobody import crypt_url, body_crypt, CryptUrlPrefix
from antiadblock.cryprox.cryprox.config.system import SEED_HEADER_NAME, PARTNER_TOKEN_HEADER_NAME


def test_config_bk(stub_server, cryprox_worker_url, get_config):
    expected_data = requests.get(urljoin(stub_server.url, '/crypted/config_bk.html')).text
    proxied_data = requests.get(urljoin(cryprox_worker_url, '/config_bk.html'),
                                headers={'host': TEST_LOCAL_HOST,
                                         SEED_HEADER_NAME: 'my2007',
                                         PARTNER_TOKEN_HEADER_NAME: get_config('test_local').PARTNER_TOKENS[0]}).text

    assert expected_data == proxied_data


def test_config_adfox(stub_server, cryprox_worker_url, get_config):
    expected_data = requests.get(urljoin(stub_server.url, '/crypted/config_adfox.html')).text
    proxied_data = requests.get(urljoin(cryprox_worker_url, '/config_adfox.html'),
                                headers={'host': TEST_LOCAL_HOST,
                                         SEED_HEADER_NAME: 'my2007',
                                         PARTNER_TOKEN_HEADER_NAME: get_config('test_local').PARTNER_TOKENS[0]}).text

    assert expected_data == proxied_data


def test_crypt_joinedlink_meta(stub_server, get_key_and_binurlprefix_from_config, get_config):
    test_config = get_config('test_local')
    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)
    expected_data = requests.get(urljoin(stub_server.url, "/crypted/pcode_json_joinedLink.js")).json()

    proxied_data = requests.get(crypt_url(binurlprefix, urljoin('http://an.yandex.ru', '/meta/pcode_json_joinedLink.js?join-show-links=1'), key, True, origin='test.local'),
                                headers={'host': TEST_LOCAL_HOST,
                                         SEED_HEADER_NAME: 'my2007',
                                         PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]}).json()
    assert expected_data == proxied_data


def test_crypt_non_base64_html_meta(get_key_and_binurlprefix_from_config, get_config):
    test_config = get_config('test_local')
    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)

    expected_data = json.loads(resource.find("resources/test_crypt_non_base64_html_meta/non_base64_html.js"))

    proxied_data = requests.get(crypt_url(binurlprefix, urljoin('http://an.yandex.ru', '/meta/non_base64_html.js?join-show-links=1'), key, True, origin='test.local'),
                                headers={'host': TEST_LOCAL_HOST,
                                         SEED_HEADER_NAME: 'my2007',
                                         PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]}).json()
    assert expected_data == proxied_data


def test_crypt_non_base64_html_2_meta(get_key_and_binurlprefix_from_config, get_config):
    test_config = get_config('test_local')
    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)

    expected_data = json.loads(resource.find("resources/test_crypt_non_base64_html_meta/non_base64_html_2.json"))

    proxied_data = requests.get(crypt_url(binurlprefix, urljoin('http://an.yandex.ru', '/meta/non_base64_html_2.json?join-show-links=1'), key, True, origin='test.local'),
                                headers={'host': TEST_LOCAL_HOST,
                                         SEED_HEADER_NAME: 'my2007',
                                         PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]}).json()
    assert expected_data == proxied_data


@pytest.mark.parametrize('ad_systems', [
    [AdSystem.BK, AdSystem.ADFOX, AdSystem.RAMBLER_SSP],
    [AdSystem.BK, AdSystem.ADFOX],
    [AdSystem.BK],
    [AdSystem.RAMBLER_SSP],
])
def test_denied_ad_systems(stub_config_server, get_key_and_binurlprefix_from_config, get_config, reload_configs, ad_systems):
    def handler(**_):
        config = json.loads(initial_config())
        config["test_local"]['config']['AD_SYSTEMS'] = ad_systems
        return {'text': json.dumps(config), 'code': 200}
    stub_config_server.set_handler(handler)
    reload_configs()
    test_config = get_config('test_local')
    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)
    urls = {AdSystem.BK: crypt_url(binurlprefix, 'http://an.yandex.ru/meta/pcode_jsonp.js?json=1', key, True, origin='test.local'),
            AdSystem.ADFOX: crypt_url(binurlprefix, 'http://ads.adfox.ru/123/getCodeTest', key, True, origin='test.local'),
            AdSystem.RAMBLER_SSP: crypt_url(binurlprefix, 'http://dsp.rambler.ru/someCode', key, True, origin='test.local'),
            }
    for ad_system, url in urls.items():
        response = requests.get(url,
                                headers={'host': TEST_LOCAL_HOST,
                                         SEED_HEADER_NAME: 'my2007',
                                         PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]})

        assert (403 == response.status_code) == (ad_system not in ad_systems)


@pytest.mark.parametrize('ad_systems', [
    [AdSystem.BK],
    [AdSystem.ADFOX],
    [AdSystem.RAMBLER_SSP],
])
def test_disabled_ad_systems_do_not_crypt_links(stub_config_server, stub_server, get_key_and_binurlprefix_from_config, get_config, reload_configs, ad_systems):
    url_to_get = 'an.yandex.ru/path_doesnt_matters'

    def config_handler(**_):
        config = json.loads(initial_config())
        config["test_local"]['config']['AD_SYSTEMS'] = ad_systems
        config["test_local"]['config']['PROXY_URL_RE'] = [url_to_get]
        return {'text': json.dumps(config), 'code': 200}

    stub_config_server.set_handler(config_handler)
    reload_configs()

    not_crypted = """<a href="http://an.yandex.ru/partner-code-bundles/some.js"></a>
           <a href="http://ads.adfox.ru/123/getCodeTest"></a>
           <a href="http://dsp.rambler.ru/someCode"></a>"""

    def content_handler(**_):

        return {'text': not_crypted, 'code': 200, 'headers': {'Content-Type': 'text/html'}}

    stub_server.set_handler(content_handler)
    crypt_url_for_ad_system = {AdSystem.BK: r'an.yandex.ru/partner-code-bundles/some\.js',
                               AdSystem.ADFOX: r'ads\.adfox\.ru/123/getCodeTest',
                               AdSystem.RAMBLER_SSP: r'dsp\.rambler\.ru/someCode'}

    crypt_url_re = [crypt_url_for_ad_system.get(ad_s) for ad_s in ad_systems]

    test_config = get_config('test_local')
    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)
    local_binurlprefix = CryptUrlPrefix('http', 'test.local', 'my2007', test_config.CRYPT_URL_PREFFIX)

    crypted = body_crypt(not_crypted, binurlprefix=local_binurlprefix, key=key, crypt_url_re=re2.compile(re_expand(crypt_url_re)), enable_trailing_slash=True)
    # test version of binurlprefix has '0.0.0.0:33266' as a netloc in common_crypt_prefix
    crypted = crypted.replace(binurlprefix.common_crypt_prefix.netloc, "test.local")

    response = requests.get(crypt_url(binurlprefix, "http://" + url_to_get, key, True, origin='test.local'),
                            headers={'host': TEST_LOCAL_HOST,
                                     SEED_HEADER_NAME: 'my2007',
                                     PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]})

    assert response.content == crypted


def handler(**request):
    headers = {'x-aab-uri-query': request.get('query', '')}
    return {'text': 'ok', 'code': 200, 'headers': headers}


@pytest.mark.parametrize('original_value, disabled_types, expected_value', [
    # Исходное значение в урле:
    # None - остуствует
    # undefined - невалидное
    # 32 - валидное, отличное от дефолтного
    # 56 - валидное, дефолтное

    # Значение в конфиге
    # None - дефолтное
    # [] - все включено
    # [DisabledAdType.TEXT.value, DisabledAdType.MEDIA.value] - не дефолтное
    [None, None, '56'],
    [None, [], None],
    [None, [DisabledAdType.TEXT.value, DisabledAdType.MEDIA.value], '3'],
    ['undefined', None, '56'],
    ['undefined', [], 'undefined'],
    ['undefined', [DisabledAdType.TEXT.value, DisabledAdType.MEDIA.value], '3'],
    ['32', None, '56'],
    ['32', [], '32'],
    ['32', [DisabledAdType.TEXT.value, DisabledAdType.MEDIA.value], '35'],
    ['56', None, '56'],
    ['56', [], '56'],
    ['56', [DisabledAdType.TEXT.value, DisabledAdType.MEDIA.value], '59'],
])
def test_disabled_ad_types(stub_server, original_value, disabled_types, expected_value, get_config, set_handler_with_config, get_key_and_binurlprefix_from_config):
    test_config = get_config('test_local')
    new_test_config = test_config.to_dict()
    if disabled_types is not None:
        new_test_config['DISABLED_AD_TYPES'] = disabled_types

    set_handler_with_config(test_config.name, new_test_config)
    stub_server.set_handler(handler)

    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)
    add_query_arg = '' if original_value is None else '&disabled-ad-type={}'.format(original_value)
    for link in ['http://an.yandex.ru/meta/123?ololol=lololol', 'http://yandex.ru/ads/meta/123?ololol=lololol', 'http://ads.adfox.ru/123/getBulk?ololol=lololol']:
        crypted_link = crypt_url(binurlprefix, link+add_query_arg, key, True, origin='test.local')

        response = requests.get(crypted_link,
                                headers={'host': TEST_LOCAL_HOST, PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]})
        if expected_value is None:
            assert 'disabled-ad-type' not in response.headers.get('x-aab-uri-query')
        else:
            assert 'disabled-ad-type={}'.format(expected_value) in response.headers.get('x-aab-uri-query')


@pytest.mark.parametrize('original_value, is_disabled, expected_value', [
    [None, False, None],
    ['0', False, '0'],
    ['1', False, '1'],
    [None, True, None],
    ['0', True, '0'],
    ['1', True, '0'],
])
@pytest.mark.parametrize('link', [
    'http://an.yandex.ru/meta/123?ololol=lololol',
    'http://yandex.ru/ads/meta/123?ololol=lololol',
    'http://ads.adfox.ru/123/getBulk?ololol=lololol',
    'http://an.yandex.ru/adfox/123/getBulk?ololol=lololol',
    'http://yandex.ru/ads/adfox/123/getBulk?ololol=lololol',
])
def test_disabled_tga_with_creatives(stub_server, get_config, set_handler_with_config, get_key_and_binurlprefix_from_config, original_value, is_disabled, expected_value, link):
    test_config = get_config('test_local')
    new_test_config = test_config.to_dict()
    if is_disabled:
        new_test_config['DISABLE_TGA_WITH_CREATIVES'] = is_disabled

    set_handler_with_config(test_config.name, new_test_config)
    stub_server.set_handler(handler)
    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)
    add_query_arg = '' if original_value is None else '&tga-with-creatives={}'.format(original_value)
    crypted_link = crypt_url(binurlprefix, link + add_query_arg, key, True, origin='test.local')

    response = requests.get(crypted_link,
                            headers={'host': TEST_LOCAL_HOST, PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]})
    if expected_value is None:
        assert 'tga-with-creatives' not in response.headers.get('x-aab-uri-query')
    else:
        assert 'tga-with-creatives={}'.format(expected_value) in response.headers.get('x-aab-uri-query')


@pytest.mark.parametrize('original_value, new_value, expected_value', [
    [None, None, None],
    ['12345', None, '12345'],
    [None, '54321', '54321'],
    ['12345', '54321', '54321'],
])
@pytest.mark.parametrize('link', [
    'http://an.yandex.ru/meta/123?ololol=lololol',
    'http://yandex.ru/ads/meta/123?ololol=lololol',
])
def test_add_aim_banner_id(stub_server, get_config, set_handler_with_config, get_key_and_binurlprefix_from_config, original_value, new_value, expected_value, link):
    test_config = get_config('test_local')
    new_test_config = test_config.to_dict()
    if new_value is not None:
        new_test_config['AIM_BANNER_ID_DEBUG_VALUE'] = new_value

    set_handler_with_config(test_config.name, new_test_config)
    stub_server.set_handler(handler)
    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)
    add_query_arg = '' if original_value is None else '&aim-banner-id={}'.format(original_value)
    crypted_link = crypt_url(binurlprefix, link + add_query_arg, key, True, origin='test.local')

    response = requests.get(crypted_link,
                            headers={'host': TEST_LOCAL_HOST, PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]})
    if expected_value is None:
        assert 'aim-banner-id' not in response.headers.get('x-aab-uri-query')
    else:
        assert 'aim-banner-id={}'.format(expected_value) in response.headers.get('x-aab-uri-query')
