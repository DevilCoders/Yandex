from urlparse import urlparse

import pytest
import requests
from hamcrest import assert_that, is_, not_none, none

from antiadblock.cryprox.cryprox.common.cryptobody import crypt_url
from antiadblock.cryprox.cryprox.config.system import ACCEL_REDIRECT_PREFIX, PARTNER_ACCEL_REDIRECT_PREFIX, PARTNER_TOKEN_HEADER_NAME, \
    EXTENSIONS_TO_ACCEL_REDIRECT, NGINX_SERVICE_ID_HEADER
from antiadblock.cryprox.tests.lib.containers_context import TEST_LOCAL_HOST


@pytest.mark.parametrize('extension,redirect', [(ext, True) for ext in tuple(EXTENSIONS_TO_ACCEL_REDIRECT) +
                                                (EXTENSIONS_TO_ACCEL_REDIRECT[-1] + '?v=1.3',)] +
                         [(ext, False) for ext in ('html', 'js', 'css', '', 'jpg.js')])
@pytest.mark.parametrize('host', ('yastatic.net', 'storage.mds.yandex.net/get-canvas-html5'))
def test_proxy_accel_redirect_extensions(host, extension, redirect, get_config, get_key_and_binurlprefix_from_config):
    """
    Test system ACCEL-REDIRECT for images
    """

    link = "https://{}/test.{}".format(host, extension)
    test_config = get_config('test_local')
    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)
    crypted_link = crypt_url(binurlprefix, link, key, True)
    response = requests.get(crypted_link, headers={'host': TEST_LOCAL_HOST,
                                                   PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]})
    assert response.headers.get(NGINX_SERVICE_ID_HEADER) == 'test_local'
    assert response.headers.get('X-Accel-Redirect') == (ACCEL_REDIRECT_PREFIX + link if redirect else None)


@pytest.mark.parametrize('link,matcher', [
    ('https://static-mon.yandex.net/static/main.js?pid=yabndex_mail', not_none),
    ('http://test.local/sad.jpg?static-mon.yandex.net/', none)])
def test_proxy_accel_redirect_security(link, matcher, get_key_and_binurlprefix_from_config, get_config):
    """
    Test ACCEL-REDIRECT only for white-listed domains, see https://st.yandex-team.ru/ANTIADB-946
    """
    test_config = get_config('test_local')
    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)
    crypted_link = crypt_url(binurlprefix, link, key, True)
    response = requests.get(crypted_link, headers={'host': TEST_LOCAL_HOST,
                                                   PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]})
    assert response.headers.get(NGINX_SERVICE_ID_HEADER) == 'test_local'
    assert_that(response.headers.get('X-Accel-Redirect'), is_(matcher()))


@pytest.mark.parametrize('link,expected_prefix', [
    ('http://avatars.mds.yandex.net/get-direct/236476/oFeLU-OuJ0_onqzIsKYdHg/y450', ACCEL_REDIRECT_PREFIX),
    ('https://storage.mds.yandex.net/get-bstor/ololo.gif', ACCEL_REDIRECT_PREFIX),
    ('https://yastatic.net/trololo.jpeg', ACCEL_REDIRECT_PREFIX),
    ('https://banners.adfox.ru/180518/adfox/575363/2523563.gif', ACCEL_REDIRECT_PREFIX),
    ('http://test.local/accel-images/test.png', PARTNER_ACCEL_REDIRECT_PREFIX),
    ('http://avatars.somedomain.ru/get-image/236476', ACCEL_REDIRECT_PREFIX)])
def test_proxy_accel_redirect_prefix(link, expected_prefix, set_handler_with_config, get_key_and_binurlprefix_from_config, get_config):
    """
    Test ACCEL-REDIRECT for partner with custom prefix
    """
    test_config = get_config('test_local')
    new_test_config = test_config.to_dict()
    new_test_config["PROXY_URL_RE"].append(r'avatars\.somedomain\.ru/.*')
    new_test_config['PROXY_ACCEL_REDIRECT_URL_RE'] = [r'^(?:https?:)?//avatars\.somedomain\.ru/']

    set_handler_with_config(test_config.name, new_test_config)
    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)
    crypted_link = crypt_url(binurlprefix, link, key, True)
    response = requests.get(crypted_link, headers={'host': TEST_LOCAL_HOST,
                                                   PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]})

    assert response.headers.get(NGINX_SERVICE_ID_HEADER) == 'test_local'
    assert response.headers.get('X-Accel-Redirect') == (expected_prefix + link)


@pytest.mark.parametrize('partner_accel_redirect', (True, False))
@pytest.mark.parametrize('link, expect_a_redirect', [
    (link, True) for link in ['http://test.local/images/test.png',
                              'http://test.local/static/test.jpeg',
                              'http://test.local/static-images/test.png?ogogo=egegei',
                              'http://test.local/any/test.tif']] + [
    (link, False) for link in ['http://test.local/images/test.png1',
                               'http://test.local/static/test.jpeg2000',
                               'http://test.local/static-images/test.png.iso?ogogo=egegei',
                               'http://test.local/any/test.tifff',
                               'http://test.local/any/test_without_ext']
])
def test_partner_accel_redirect(link, partner_accel_redirect, expect_a_redirect, set_handler_with_config, get_key_and_binurlprefix_from_config, get_config):
    test_config = get_config('test_local')
    new_test_config = test_config.to_dict()
    new_test_config['PARTNER_ACCEL_REDIRECT_ENABLED'] = partner_accel_redirect
    new_test_config['ACCEL_REDIRECT_URL_RE'] = []

    set_handler_with_config(test_config.name, new_test_config)

    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)
    crypted_link = crypt_url(binurlprefix, link, key, True)
    response = requests.get(crypted_link, headers={'host': TEST_LOCAL_HOST,
                                                   PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]})

    assert response.headers.get(NGINX_SERVICE_ID_HEADER) == 'test_local'
    assert response.headers.get('X-Accel-Redirect') == ((PARTNER_ACCEL_REDIRECT_PREFIX + link) if all([expect_a_redirect, partner_accel_redirect]) else None)


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
def test_accel_redirect_http_proto(link_proto, x_forwarded_proto_header, expected_proto, get_key_and_binurlprefix_from_config, get_config):
    """
    Test ACCEL-REDIRECT url proto with various X-Forwarded-Proto and origin url proto values
    :param link_proto: original link proto before crypt
    :param x_forwarded_proto_header: X-Forwarded-Proto  header
    :param expected_proto: expected link proto in accel-redirect header
    """

    test_config = get_config('test_local')
    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)
    req_headers = {'host': TEST_LOCAL_HOST, PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]}
    req_headers.update(x_forwarded_proto_header)

    link = '{}//test.local/accel-images/test.png'.format(link_proto + ':' if link_proto else '')
    crypted_link = crypt_url(binurlprefix, link, key, True)
    if urlparse(crypted_link).scheme == '':
        crypted_link = 'http:' + crypted_link
    response = requests.get(crypted_link, headers=req_headers)
    assert response.headers.get(NGINX_SERVICE_ID_HEADER) == 'test_local'
    assert response.headers.get('X-Accel-Redirect').find('/{}://test.local'.format(expected_proto)) > 0
