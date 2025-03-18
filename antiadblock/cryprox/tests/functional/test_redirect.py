# -*- coding: utf8 -*-
import pytest
import requests
from urlparse import urlparse

from hamcrest import assert_that, equal_to

from antiadblock.cryprox.cryprox.common.cryptobody import crypt_url
from antiadblock.cryprox.cryprox.config import system as system_config
from antiadblock.cryprox.tests.lib.containers_context import TEST_LOCAL_HOST


def handler(**request):
    path = request.get('path', '/')
    if '/redirect' in path or '/pcode/adfox/header-bidding.js' in path:
        return {'text': 'link', 'code': 302, 'headers': {'Location': 'http://test.local/followed_to_redirect'}}
    if path == '/followed_to_redirect':
        return {'text': 'successfully followed', 'code': 200}
    return {}


@pytest.mark.parametrize('test_url,expected_response_code',
                         [('http://test.local/redirect', 200),  # Тест применения FOLLOW_REDIRECT_URL_RE из конфига партнера
                          ('http://yastatic.net/redirect', 302),
                          ('http://banners.adfox.ru/redirect', 302),
                          ('http://ads.adfox.ru/12345/getBulk/redirect', 200),  # Из config/adfox.py
                          ('http://yastatic.net/pcode/adfox/header-bidding.js', 200),  # Из config/adfox.py
                          ])
def test_follow_redirects(stub_server, get_config, set_handler_with_config,
                          get_key_and_binurlprefix_from_config, test_url, expected_response_code):
    """
    Test follow_redirect_url_re
    """
    test_config = get_config('test_local')
    new_test_config = test_config.to_dict()
    new_test_config['FOLLOW_REDIRECT_URL_RE'] = [r'test\.local/redirect']
    set_handler_with_config(test_config.name, new_test_config)

    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)

    stub_server.set_handler(handler)

    crypted_link = crypt_url(binurlprefix, test_url, key)
    headers = {'host': TEST_LOCAL_HOST, system_config.PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]}
    response = requests.get(crypted_link, headers=headers, allow_redirects=False)
    assert response.headers.get(system_config.NGINX_SERVICE_ID_HEADER) == 'test_local'
    assert response.status_code == expected_response_code
    if expected_response_code == 302:
        assert response.headers['Location'] == 'http://test.local/followed_to_redirect'
    elif expected_response_code == 200:
        assert 'Location' not in response.headers
        assert response.text == 'successfully followed'


@pytest.mark.usefixtures('stub_server')
def test_redirect(get_key_and_binurlprefix_from_config, get_config, set_handler_with_config):
    """
    Test redirect decorator
    """
    test_config = get_config('test_local')
    new_test_config = test_config.to_dict()
    new_test_config['CLIENT_REDIRECT_URL_RE'] = [r'test\.local/maaan/.*']
    set_handler_with_config(test_config.name, new_test_config)

    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)

    redirect_urls = ('//test.local/maaan/kulichiki.gif',
                     'http://direct.yandex.ru/?partner')  # check that decorator 'redirect' is working before 'accel_redirect'

    test_cases = [(url, True) for url in redirect_urls] + [(url, False) for url in ['http://an.yandex.ru/file.txt', '//test.local/kulichiki.gif']]

    for test_url, is_redirected in test_cases:
        crypted_link = crypt_url(binurlprefix, test_url, key)
        if not urlparse(crypted_link).scheme:
            crypted_link = 'http:' + crypted_link

        response = requests.get(crypted_link, headers={'host': TEST_LOCAL_HOST, system_config.PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]}, allow_redirects=False)
        assert response.headers.get(system_config.NGINX_SERVICE_ID_HEADER) == 'test_local'
        assert response.status_code == (302 if is_redirected else 404)  # else url is not supported from cryptoproxy


@pytest.mark.parametrize('link_proto,x_forwarded_proto_header,expected_proto',
                         [('http', {'X-Forwarded-Proto': 'https'}, 'http'),
                          ('http', {}, 'http'),
                          ('http', {'X-Forwarded-Proto': 'http'}, 'http'),
                          ('https', {'X-Forwarded-Proto': 'http'}, 'https'),
                          ('https', {}, 'https'),
                          ('https', {'X-Forwarded-Proto': 'https'}, 'https'),
                          ('', {'X-Forwarded-Proto': 'http'}, 'http'),
                          ('', {}, 'http'),
                          ('', {'X-Forwarded-Proto': 'https'}, 'https'),
                          ])
@pytest.mark.usefixtures('stub_server')
def test_redirect_http_proto(link_proto, x_forwarded_proto_header, expected_proto, get_key_and_binurlprefix_from_config, get_config):
    """
    Test that REDIRECT url protocol with various X-Forwarded-Proto and origin url proto values
    :param link_proto: original link proto before crypt
    :param x_forwarded_proto_header: X-Forwarded-Proto header
    :param expected_proto: expected link proto in Location header
    """

    test_config = get_config('test_local')
    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)
    req_headers = {'host': TEST_LOCAL_HOST, system_config.PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]}
    req_headers.update(x_forwarded_proto_header)

    link = '{}//direct.yandex.ru/?partner'.format(link_proto + ':' if link_proto else link_proto)
    crypted_link = crypt_url(binurlprefix, link, key)
    if not urlparse(crypted_link).scheme:
        crypted_link = 'http:' + crypted_link
    response = requests.get(crypted_link, headers=req_headers, allow_redirects=False)
    assert response.headers.get(system_config.NGINX_SERVICE_ID_HEADER) == 'test_local'
    assert response.status_code == 302
    assert response.headers.get('Location') == '{}://direct.yandex.ru/?partner'.format(expected_proto)


@pytest.mark.parametrize('url,expected_location', [
    ('http://test.local/ru', 'http://test.local/путь-с-кириллицей'),
    ('http://test.local/eng', 'http://test.local/non-cyrillic-path'),
])
def test_redirect_with_cyrillic_location(stub_server, get_config, get_key_and_binurlprefix_from_config, url, expected_location):

    def handler(**_):
        return {'text': 'Success', 'code': 302, 'headers': {'Location': expected_location}}

    stub_server.set_handler(handler)
    test_config = get_config('test_local')
    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)

    crypted_url = crypt_url(binurlprefix, url, key, True, origin='test.local')
    response = requests.get(crypted_url,
                            headers={'host': 'test.local',
                                     system_config.PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]},
                            allow_redirects=False)

    assert response.status_code == 302
    assert response.headers.get('Location') == expected_location


@pytest.mark.parametrize('location, is_naydex, is_bypass, expected', (
    ('http://an.yandex.ru/system/context_adb.js', False, False,
     'http://test.local/XWtN9S538/my2007kK/Kdf7P9al87f/5dRTH-B/neCp_J/QDEf_/71cQv4B/2_7rltCU1x/finr9/IH_jnnlS/TX_RN4n/VdzOvqfA/a4guRDZdAKc/'),
    ('http://yandex.ru/ads/system/context_adb.js', False, False,
     'http://test.local/XWtN9S647/my2007kK/Kdf7P9akc0P/4pVWjWW/kOG67c/hfG_X/8xMxtrB/G-9KhwBWZP/ey-ns/JjTjmflS/ijvWcUp/W9v0r7HH/McotSDRQM6eJ/'),
    ('http://yastatic.net/pcode-native-bundles/12345/widget_adb.js', True, False,
     'http://test-local.naydex.net/test/SNXt12263/my2007kK/Kdf7P9akc0I/ppRVnKH/y6C-_Z/QAC-P/r1YRu4g/a47Lk4H2d-/fieg7/d2R4wuQP/kXHf_MH/eeblurDW/a44/yeAhwHrqJpl/3AlQx/wITSdR/Ax3i4avt9bk/'),
    ('http://yastatic.net/pcode-native-bundles/12345/widget_adb.js', False, True,
     'http://yastatic.net/pcode-native-bundles/12345/widget_adb.js'),
    ('http://test.local/some_url', False, False,
     'http://test.local/some_url'),
))
def test_redirect_with_crypted_location(stub_server, get_config, set_handler_with_config, get_key_and_binurlprefix_from_config,
                                        location, is_naydex, is_bypass, expected):

    def handler(**_):
        return {'text': 'Success', 'code': 302, 'headers': {'Location': location}}

    stub_server.set_handler(handler)
    config_name = 'test_local'
    test_config = get_config(config_name)
    new_test_config = test_config.to_dict()
    if is_naydex:
        new_test_config['ENCRYPT_TO_THE_TWO_DOMAINS'] = True
        new_test_config['PARTNER_COOKIELESS_DOMAIN'] = 'naydex.net'
    if is_bypass:
        new_test_config['BYPASS_URL_RE'] = 'yastatic\.net/.*?'
    set_handler_with_config(config_name, new_test_config)

    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)

    url = "http://an.yandex.ru/system/adfox_adb.js"
    crypted_url = crypt_url(binurlprefix, url, key, True, origin='test.local')
    response = requests.get(crypted_url,
                            headers={'host': 'test.local',
                                     system_config.PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]},
                            allow_redirects=False)
    assert response.status_code == 302
    assert_that(response.headers.get('Location'), equal_to(expected))
