# -*- coding: utf8 -*-
import cgi
import json
from urlparse import urljoin

import pytest
import requests
from hamcrest import assert_that, has_entries, has_key, is_not, equal_to

from antiadblock.cryprox.cryprox.common.cryptobody import crypt_url
from antiadblock.cryprox.cryprox.config.config import adfox_config
from antiadblock.cryprox.cryprox.config.bk import DENY_HEADERS_BACKWARD_RE
from antiadblock.cryprox.cryprox.config.system import DENY_HEADERS_BACKWARD_PROXY_RE, PROXY_SERVICE_HEADER_NAME, PARTNER_TOKEN_HEADER_NAME, DENY_HEADERS_FORWARD_RE, NGINX_SERVICE_ID_HEADER, SEED_HEADER_NAME
from antiadblock.cryprox.tests.lib.containers_context import AUTO_RU_HOST, TEST_LOCAL_HOST


def handler(**request):
    if request.get('path', '/') == '/header_X_AAB_PROXY':
        if request['headers'].get('HTTP_X_AAB_PROXY', '') == '1':
            return {'text': 'ok', 'code': 200}
        else:
            return {'text': 'fail', 'code': 400}
    elif request.get('path', '/') == '/123/getCode/header_X_Adfox_Debug':
        if request['headers'].get('HTTP_X_ADFOX_DEBUG', '') == '1':
            return {'text': 'ok', 'code': 200}
        else:
            return {'text': 'fail', 'code': 400}
    elif request.get('path', '/') == '/123/getCode/Adfox-S2s-Key':
        if request['headers'].get('http_x_adfox_s2s_key'.upper(), '') == '9118308262628211612':
            return {'text': 'ok', 'code': 200}
        else:
            return {'text': 'fail', 'code': 400}
    elif request.get('path', '/') == '/bk-headers':
        return {'text': 'ok', 'code': 200,
                'headers': {'Set-Cookie': 'language=en;Domain=.an.yandex.ru;Path=/;Max-Age=20',
                            'Access-Control-Allow-Credentials': 'true'}}
    elif request.get('path', '/') == '/proxy-headers':
        return {'text': 'ok', 'code': 200,
                'headers': {'Strict-Transport-Security': 'max-age=43200000; includeSubDomains'}}
    return {}


@pytest.mark.parametrize('headers,expected_code,expected_text', [
    ({}, 400, 'fail'),
    ({PROXY_SERVICE_HEADER_NAME: '1'}, 200, 'ok')])
def test_nginx_header_x_aab_proxy(stub_server, headers, expected_code, expected_text):
    """
    Test nginx configuration.
    Nginx responce 'ok' if 'X-AAB-Proxy: 1' headers present, else 'fail'.
    """
    stub_server.set_handler(handler)
    response = requests.get(urljoin(stub_server.url, '/header_X_AAB_PROXY'), headers=headers)

    assert response.status_code == expected_code
    assert response.text == expected_text


def test_proxy_header_x_aab_proxy(stub_server, cryprox_worker_url, get_config):
    """
    Test proxy header.
    Nginx responce 'ok' if 'X-AAB-Proxy: 1' headers present, else 'fail'.
    """
    stub_server.set_handler(handler)
    expected_data = 'ok'
    response = requests.get(urljoin(cryprox_worker_url, '/header_X_AAB_PROXY'),
                            headers={'host': TEST_LOCAL_HOST,
                                     PARTNER_TOKEN_HEADER_NAME: get_config('test_local').PARTNER_TOKENS[0]})
    assert response.headers.get(NGINX_SERVICE_ID_HEADER) == 'test_local'
    assert response.text == expected_data


@pytest.mark.parametrize('headers,expected_code,expected_text', [
    ({}, 400, 'fail'),
    ({'X-Adfox-Debug': '1'}, 200, 'ok')])
def test_stub_header_x_adfox_debug(stub_server, headers, expected_code, expected_text):
    """
    Test nginx configuration.
    Nginx responce 'ok' if 'X-Adfox-Debug: 1' headers present, else 'fail'.
    """
    stub_server.set_handler(handler)
    response = requests.get(urljoin(stub_server.url, '/123/getCode/header_X_Adfox_Debug'), headers=headers)

    assert response.status_code == expected_code
    assert response.text == expected_text


@pytest.mark.parametrize('partner,expected_code,expected_text', [
    ("test_local", 200, 'ok'),
    ('autoru', 400, 'fail')])
def test_proxy_header_x_adfox_debug(stub_server, partner, expected_code, expected_text, get_key_and_binurlprefix_from_config, get_config):
    """
    Test proxy header.
    Nginx responce 'ok' if 'X-Adfox-Debug: 1' headers present, else 'fail'.
    """
    test_config = get_config(partner)
    host = TEST_LOCAL_HOST if partner == "test_local" else AUTO_RU_HOST
    headers = {'host': host, PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]}
    stub_server.set_handler(handler)
    key, binurlprefix = get_key_and_binurlprefix_from_config(partner)
    crypted_link = crypt_url(binurlprefix, 'http://ads.adfox.ru/123/getCode/header_X_Adfox_Debug', key, origin='test.local')
    response = requests.get(crypted_link, headers=headers)
    assert response.headers.get(NGINX_SERVICE_ID_HEADER) == partner
    assert response.status_code == expected_code
    assert response.text == expected_text


@pytest.mark.usefixtures('stub_server')
def test_lack_of_etag_header(cryprox_worker_url, get_config):
    response = requests.get(urljoin(cryprox_worker_url, 'json.html'),
                            headers={"host": AUTO_RU_HOST, PARTNER_TOKEN_HEADER_NAME: get_config('autoru').PARTNER_TOKENS[0]})
    assert response.headers.get(NGINX_SERVICE_ID_HEADER) == 'autoru'
    assert response.status_code == 200
    assert 'etag' not in (header.lower() for header in response.headers.keys())


def test_content_length_header_for_bin_file(stub_server, cryprox_worker_url, get_config):
    """
    Bin file will not be changed by service, so Content-Length is constant
    """
    expected_data = requests.get(urljoin(stub_server.url, 'test.bin'))
    proxied_data = requests.get(urljoin(cryprox_worker_url, 'test.bin'),
                                headers={"host": AUTO_RU_HOST,
                                         PARTNER_TOKEN_HEADER_NAME: get_config('autoru').PARTNER_TOKENS[0]})
    assert proxied_data.headers.get(NGINX_SERVICE_ID_HEADER) == 'autoru'
    assert expected_data.headers['Content-Length'] == proxied_data.headers['Content-Length']
    assert int(proxied_data.headers['Content-Length']) == len(proxied_data.text)


@pytest.mark.usefixtures('stub_server')
def test_content_length_header_for_html_file(cryprox_worker_url, get_config):
    response = requests.get(urljoin(cryprox_worker_url, 'json.html'),
                            headers={"host": AUTO_RU_HOST,
                                     PARTNER_TOKEN_HEADER_NAME: get_config('autoru').PARTNER_TOKENS[0]})
    assert response.headers.get(NGINX_SERVICE_ID_HEADER) == 'autoru'
    assert response.status_code == 200
    assert int(response.headers['Content-Length']) == len(response.text)


@pytest.mark.parametrize('file_name, expected_type', [
    ('index.html', 'text/html'),
    ('script.js', 'application/javascript'),
    ('CSSheets.css', 'text/css')])
@pytest.mark.usefixtures('stub_server')
def test_content_type_header_from_cryptoproxy(file_name, expected_type, cryprox_worker_url, get_config):
    mimetype, options = cgi.parse_header(requests.get(
        urljoin(cryprox_worker_url, file_name),
        headers={"host": AUTO_RU_HOST, PARTNER_TOKEN_HEADER_NAME: get_config('autoru').PARTNER_TOKENS[0]}).headers['Content-Type'])

    assert expected_type == mimetype


# Image to us returns nginx (accel-redirect)
@pytest.mark.parametrize('file_name, expected_type', [
    ('image.png', 'image/png'),
    ('image.bmp', 'image/x-ms-bmp')])
def test_content_type_header_from_stub(stub_server, file_name, expected_type):
    stub_server.set_handler(handler)
    response = requests.get(urljoin(stub_server.url, file_name))
    assert response.status_code == 200

    mimetype, options = cgi.parse_header(response.headers['Content-Type'])

    assert expected_type == mimetype


@pytest.mark.parametrize('path, expected_headers', (('/bk-headers', DENY_HEADERS_BACKWARD_RE),
                                                    ('/proxy-headers', DENY_HEADERS_BACKWARD_PROXY_RE)))
def test_nginx_add_headers(stub_server, path, expected_headers):
    """
    Test nginx configuration.
    Nginx must send headers 'Set-Cookie', 'Access-Control-Allow-Credentials' from /bk-headers endpoint
    and send headers 'Strict-Transport-Security' from /proxy-headers endpoint
    """
    stub_server.set_handler(handler)
    response = requests.get(urljoin(stub_server.url, path))

    assert response.status_code == 200
    assert response.text == 'ok'
    response_headers = [header.lower() for header in response.headers.keys()]
    assert all(key.lower() in response_headers for key in expected_headers)


@pytest.mark.parametrize('url, expected_headers', (('http://an.yandex.ru/bk-headers', DENY_HEADERS_BACKWARD_RE),
                                                   ('http://yabs.yandex.ru/bk-headers', DENY_HEADERS_BACKWARD_RE),
                                                   ('http://yastatic.net/proxy-headers', DENY_HEADERS_BACKWARD_PROXY_RE),
                                                   ('http://ads.adfox.ru/proxy-headers', DENY_HEADERS_BACKWARD_PROXY_RE),
                                                   ('http://an.yandex.ru/proxy-headers', DENY_HEADERS_BACKWARD_PROXY_RE),
                                                   ('http://yabs.yandex.ru/proxy-headers', DENY_HEADERS_BACKWARD_PROXY_RE)))
def test_remove_headers(url, expected_headers, get_key_and_binurlprefix_from_config, get_config, stub_server):
    stub_server.set_handler(handler)

    test_config = get_config('autoru')
    crypt_key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)
    crypted_link = crypt_url(binurlprefix, url, crypt_key, origin='test.local')
    response = requests.get(crypted_link, headers={"host": AUTO_RU_HOST,
                                                   PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]})
    assert response.headers.get(NGINX_SERVICE_ID_HEADER) == 'autoru'
    assert response.status_code == 200
    assert response.text == 'ok'
    response_headers = [header.lower() for header in response.headers.keys()]
    assert all(key.lower() not in response_headers for key in expected_headers)


@pytest.mark.parametrize('url, expected_headers',
                         (('http://ads.adfox.ru/bk-headers', DENY_HEADERS_BACKWARD_RE),  # adfox
                          ('http://yastatic.net/bk-headers', DENY_HEADERS_BACKWARD_RE),  # proxy
                          ('http://auto.ru/bk-headers', DENY_HEADERS_BACKWARD_RE),  # partner
                          ('http://auto.ru/proxy-headers', DENY_HEADERS_BACKWARD_PROXY_RE)))  # partner
def test_not_remove_bk_headers_for_other_clients(url, expected_headers, get_key_and_binurlprefix_from_config, get_config, stub_server):
    stub_server.set_handler(handler)

    test_config = get_config('autoru')
    crypt_key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)
    crypted_link = crypt_url(binurlprefix, url, crypt_key, origin='test.local')
    response = requests.get(crypted_link, headers={"host": AUTO_RU_HOST, PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]})

    assert response.status_code == 200
    assert response.text == 'ok'
    response_headers = [header.lower() for header in response.headers.keys()]
    assert all(key.lower() in response_headers for key in expected_headers)


adfox_click_url = 'http://ads.adfox.ru/255821/goLink?puid63=yes&puid2=belle-et-sebastien&hash=9b5b3083780867cd&sj=8yaswLjvl2pBsF17RKzU9GQY8ozz7EI3WxVmN6ijtsuixEaOpAchfRTaAh5-bc1-DD74jHNbXKPGlHZvm' + \
                  '2Xrwqj68C8vZjDFTujRbo4%3D&ad-session-id=9953551536078319901&p2=fxlx&ybv=0.990&ylv=0.990&ytt=562949953422885&pr=drifpvv&p1=cbbqe&p5=fusal&rand=cvszhuy&rqs=Y66cFVpuXAr2sY5bNWJmMw' + \
                  'lPnXL1koF3&pf=https%3A%2F%2Fafisha.yandex.ru%2Fevents%2F59f753246a1eb524510b6a06'


@pytest.mark.parametrize('link, s2s_key_header_expected',
                         (('http://ads.adfox.ru/123/getCode/', True),  # adfox
                          ('http://an.yandex.ru/adfox/123/getBulk/', False),
                          (adfox_click_url, True),
                          ('https://banners.adfox.ru/180820/adfox/829601/2634051.jpg', False),  # adfox
                          ('http://auto.ru/123/getCode/', False),  # partner
                          ('http://an.yandex.ru/meta/123/', False),  # bk
                          ('http://yabs.yandex.ru/count/', False),  # bk
                          ('http://direct.yandex.ru/123/getCode/', False)))  # proxy
@pytest.mark.usefixtures('stub_server')
def test_x_adfox_s2s_key_header(stub_server, get_key_and_binurlprefix_from_config, get_config, link, s2s_key_header_expected):

    def echo_handler(**request):
        headers = {'x-aab-uri': request['path'] + '?' + request.get('query', '')}
        headers.update(request['headers'])
        return {'text': 'ok', 'code': 200, 'headers': headers}

    stub_server.set_handler(echo_handler)
    test_config = get_config('autoru')
    crypt_key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)
    crypted_link = crypt_url(binurlprefix, link, crypt_key, origin='test.local')
    response = requests.get(crypted_link, headers={"host": AUTO_RU_HOST,
                                                   PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]})

    assert response.status_code == 200
    if s2s_key_header_expected:
        assert response.headers['http_x_adfox_s2s_key'] == adfox_config.ADFOX_S2SKEY
    else:
        assert 'http_x_adfox_s2s_key' not in response.headers


def test_deny_headers_forward(stub_server, get_key_and_binurlprefix_from_config, get_config):
    headers_forward = dict()

    def handler(**request):
        headers_forward.update(request['headers'])
        return {'text': 'ok', 'code': 200}

    stub_server.set_handler(handler)

    test_config = get_config('autoru')
    crypt_key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)
    crypted_link = crypt_url(binurlprefix, 'http://an.yandex.ru/123/getCode/', crypt_key, origin='test.local')

    deny_headers = [h.lower() for h in DENY_HEADERS_FORWARD_RE]
    headers = {h: 'value of ' + h for h in deny_headers}
    headers.update({'a': '1', 'b': '2'})  # arbitrary partner headers
    headers.update({'host': AUTO_RU_HOST, PARTNER_TOKEN_HEADER_NAME.lower(): test_config.PARTNER_TOKENS[0]})
    assert requests.get(crypted_link, headers=headers).status_code == 200

    for h in headers:
        formatted_header = 'HTTP_' + h.replace('-', '_').upper()
        if h == 'host':
            # header Host есть в любом запросе, когда мы удаляем этот заголовок, правильное значение выствляется Tornado
            # (удаляем потому что он может не совпадать с хостом расшифрованного запроса и быть некорректным)
            assert 'an.yandex.ru' == headers_forward[formatted_header]
        elif h in deny_headers:
            assert formatted_header not in headers_forward
        else:
            assert headers[h] == headers_forward[formatted_header]


@pytest.mark.parametrize('link_parameter,expected_crypted_link_parameter, is_crypt_links', [
    ("<https://yastatic.net/s3/fiji-static/podb/assets/desktop/ru-b4f6fc48.css>; rel=prefetch",
     "<http://test.local/SNXt13898/my2007kK/Kdf_roahEsM/J1EQ2-N/huC17M/9fG7-/g1sBq6l/-i7r1hFHE_/aiSh_/N3BokvBf/xmfcvIT/d-bVq_vG/MMk/jEzEHOZvi0S/HqoTZ/hChCvc/n1UtqyJks/fPFRv4o/dzGY-IejsU>; rel=prefetch",
     True),
    ("<https://yastatic.net>; rel=preconnect, <https://avatars.mds.yandex.net>; rel=preconnect",
     "<http://test.local/XWtN6S486/my2007kK/Kdf_roahEsM/J1EQ2-N/huC17M/8vN83/O8vZP0T/uW05JhGGFk/NCeq_ZPMjmc>; rel=preconnect, <http://test.local/XWtN7S903/my2007kK/Kdf_roahE0J/49EQ2mX/y6O_-p/UJCeL/r1dEu7R/elxYNUPFBP/VRmM2/bvupV3Xf/0TcefQBcM3l>; rel=preconnect",
     True),
    ("<https://yastatic.net/s3/fiji-static/podb/assets/desktop/ru-53eb691c.js>; rel=prefetch, <https://yastatic.net/s3/fiji-static/podb/assets/desktop/ru-0bf86f95.js>; rel=prefetch",
     "<https://yastatic.net/s3/fiji-static/podb/assets/desktop/ru-53eb691c.js>; rel=prefetch, <https://yastatic.net/s3/fiji-static/podb/assets/desktop/ru-0bf86f95.js>; rel=prefetch",
     False),
    ("<https://yastatic.net>; rel=preconnect, <https://yastatic.not.match.ru>; rel=preconnect",
     "<http://test.local/XWtN6S486/my2007kK/Kdf_roahEsM/J1EQ2-N/huC17M/8vN83/O8vZP0T/uW05JhGGFk/NCeq_ZPMjmc>; rel=preconnect, <https://yastatic.not.match.ru>; rel=preconnect",
     True)
])
def test_crypt_link_header(stub_server, set_handler_with_config, cryprox_worker_url, get_config, link_parameter, expected_crypted_link_parameter, is_crypt_links):
    def handler(**_):
        return {'code': 200, 'body': "text", 'headers': {'Link': link_parameter}}

    config = get_config('test_local')
    test_config = config.to_dict()
    test_config['CRYPT_LINK_HEADER'] = is_crypt_links
    test_config['CRYPT_ENABLE_TRAILING_SLASH'] = False
    test_config['CRYPT_URL_RE'] = [b'avatars\.mds\.yandex\.net', b'yastatic\.net.*?']

    stub_server.set_handler(handler)
    set_handler_with_config(config.name, test_config)

    response = requests.get(cryprox_worker_url, headers={'host': TEST_LOCAL_HOST,
                                                         SEED_HEADER_NAME: 'my2007',
                                                         PARTNER_TOKEN_HEADER_NAME: config.PARTNER_TOKENS[0]})

    assert response.headers.get('link') == expected_crypted_link_parameter


@pytest.mark.usefixtures("stub_server")
@pytest.mark.parametrize('host, headers_should_be_changed', [
    ('test.local', False),
    ('test.local/match', True)])
def test_update_response_headers_values(stub_server, set_handler_with_config, cryprox_worker_url, get_config, host, headers_should_be_changed):
    def handler(**_):
        return {'text': "success", 'code': 200, 'headers': {'specific_key': 'old_value'}}

    config = get_config('test_local')
    test_config = config.to_dict()
    stub_server.set_handler(handler)

    response = requests.get(cryprox_worker_url, headers={'host': host, PARTNER_TOKEN_HEADER_NAME: config.PARTNER_TOKENS[0]})

    assert response.status_code == 200
    assert_that(response.headers, has_entries(specific_key='old_value'))
    assert_that(response.headers, is_not(has_key('new_header')))

    test_config['UPDATE_RESPONSE_HEADERS_VALUES'] = {
        r'(.*?)match(.*?)': {'specific_key': 'new_value', 'new_header': 'new_header_value'}}
    set_handler_with_config(config.name, test_config)

    response = requests.get(cryprox_worker_url, headers={'host': host, PARTNER_TOKEN_HEADER_NAME: config.PARTNER_TOKENS[0]})

    assert response.status_code == 200
    if headers_should_be_changed:
        assert_that(response.headers, has_entries(specific_key='new_value'))
        assert_that(response.headers, has_entries(new_header='new_header_value'))
    else:
        assert_that(response.headers, has_entries(specific_key='old_value'))
        assert_that(response.headers, is_not(has_key('new_parameter')))


@pytest.mark.usefixtures("stub_server")
def test_update_response_with_empty_headers(stub_server, set_handler_with_config, cryprox_worker_url, get_config):
    def handler(**_):
        return {'text': "success", 'code': 200}

    config = get_config('test_local')
    test_config = config.to_dict()
    stub_server.set_handler(handler)

    response = requests.get(cryprox_worker_url, headers={'host': TEST_LOCAL_HOST,
                                                         PARTNER_TOKEN_HEADER_NAME:
                                                             config.PARTNER_TOKENS[0]})

    assert response.status_code == 200
    assert_that(response.headers, is_not(has_key('new_header')))

    test_config['UPDATE_RESPONSE_HEADERS_VALUES'] = {
        r'(.*?)': {'new_header': 'new_header_value'}}
    set_handler_with_config(config.name, test_config)

    response = requests.get(cryprox_worker_url, headers={'host': TEST_LOCAL_HOST,
                                                         PARTNER_TOKEN_HEADER_NAME:
                                                             config.PARTNER_TOKENS[0]})

    assert response.status_code == 200
    assert_that(response.headers, has_entries(new_header='new_header_value'))


@pytest.mark.usefixtures("stub_server")
@pytest.mark.parametrize('url, headers_should_be_changed', [
    ('test.local/1.html', False),
    ('test.local/match.html', True)])
def test_update_request_header_values(stub_server, get_key_and_binurlprefix_from_config, set_handler_with_config, get_config, url, headers_should_be_changed):

    def handler(**request):
        req_headers = {key.replace('HTTP_', '').replace('_', '-').lower(): val for key, val in request['headers'].items()}
        return {'text': json.dumps(req_headers), 'code': 200}

    partner_name = 'test_local'
    config = get_config(partner_name)
    test_config = config.to_dict()
    stub_server.set_handler(handler)

    test_config['UPDATE_REQUEST_HEADER_VALUES'] = {
        r'(.*?)match(.*?)': {'header1': 'newValue1', 'header2': '', 'header3': 'value3'}}
    set_handler_with_config(config.name, test_config)

    key, binurlprefix = get_key_and_binurlprefix_from_config(partner_name)
    crypted_url = crypt_url(binurlprefix, "http://{}".format(url), key, True, origin='test.local')

    headers = {'host': 'test.local', PARTNER_TOKEN_HEADER_NAME: config.PARTNER_TOKENS[0],
               'header1': 'value1', 'header2': 'value2'}

    response = requests.get(crypted_url, headers=headers)

    assert response.status_code == 200
    new_headers = json.loads(response.text)
    if headers_should_be_changed:
        assert_that(new_headers, has_entries({'header1': 'newValue1', 'header3': 'value3'}))
    else:
        assert_that(new_headers, has_entries({'header1': 'value1', 'header2': 'value2'}))


@pytest.mark.parametrize('request_header_value,expected_header_value,url', [
    (None, 'test_strm_token', 'strm.yandex.net/int/enc/srvr.xml'),
    ('strm_token', 'strm_token', 'strm.yandex.net/int/enc/srvr.xml'),
    (None, '', 'test.local/1.html'),
])
def test_update_strm_header_token_value(stub_server, get_key_and_binurlprefix_from_config, set_handler_with_config, get_config, request_header_value, expected_header_value, url):

    def handler(**request):
        strm_header_value = request['headers'].get('HTTP_X_STRM_ANTIADBLOCK', '')
        return {'text': strm_header_value, 'code': 200}

    partner_name = 'test_local'
    config = get_config(partner_name)
    test_config = config.to_dict()
    stub_server.set_handler(handler)

    test_config['CRYPT_URL_RE'] = [r'strm\.yandex\.net/int/enc/srvr\.xml']
    set_handler_with_config(config.name, test_config)

    key, binurlprefix = get_key_and_binurlprefix_from_config(partner_name)
    crypted_url = crypt_url(binurlprefix, "http://{}".format(url), key, True, origin='test.local')

    headers = {'host': 'test.local', PARTNER_TOKEN_HEADER_NAME: config.PARTNER_TOKENS[0]}

    if request_header_value is not None:
        headers['X-Strm-Antiadblock'] = request_header_value

    response = requests.get(crypted_url, headers=headers)

    assert response.status_code == 200
    assert_that(response.text, equal_to(expected_header_value))
