import requests
import pytest
from urlparse import urljoin, urlparse

from antiadblock.cryprox.cryprox.common.cryptobody import crypt_url
from antiadblock.cryprox.cryprox.config import system as system_config
from antiadblock.cryprox.tests.lib.containers_context import TEST_LOCAL_HOST, DEFAULT_COOKIELESS_HOST


def handler(**request):
    return {'text': request.get('query', ''), 'code': 200}


@pytest.mark.parametrize('original_query, expected_query', [
    ('queryarg1=testvalue&amp;queryarg2=testvalue2', 'queryarg1=testvalue&queryarg2=testvalue2'),
    ('queryarg1=testvalue&amp;queryarg2=testvalue2&amp;queryarg3=testvalue3', 'queryarg1=testvalue&queryarg2=testvalue2&queryarg3=testvalue3'),
    ('queryarg1=testvalue&nbsp;&queryarg2=testvalue2', 'queryarg1=testvalue%C2%A0&queryarg2=testvalue2'),
    ('queryarg1=testvalue&nbsp;&amp;queryarg2=testvalue2', 'queryarg1=testvalue%C2%A0&queryarg2=testvalue2'),
    ('queryarg1=testvalue&queryarg2=test&nbsp;value2', 'queryarg1=testvalue&queryarg2=test%C2%A0value2'),
    ('queryarg1=testvalue&gt;&amp;queryarg2=testvalue2', 'queryarg1=testvalue>&queryarg2=testvalue2'),
    ('queryarg1=testvalue&lt;&amp;queryarg2=testvalue2', 'queryarg1=testvalue<&queryarg2=testvalue2'),
    ('queryarg1=&quot;testvalue&quot;&queryarg2=testvalue2', 'queryarg1="testvalue"&queryarg2=testvalue2'),
    ('queryarg1=testvalue&amp;amp=1&queryarg2=testvalue2', 'queryarg1=testvalue&amp=1&queryarg2=testvalue2'),
    ('queryarg1=testvalue&amp=1&queryarg2=testvalue2', 'queryarg1=testvalue&amp=1&queryarg2=testvalue2'),
    ('queryarg1=testvalueamp&queryarg2=testvalue2', 'queryarg1=testvalueamp&queryarg2=testvalue2'),
    ('queryarg1=testvalue&ampqueryarg2=testvalue2', 'queryarg1=testvalue&ampqueryarg2=testvalue2'),
    ('queryarg1=testvalue&queryarg2=testvalue2', 'queryarg1=testvalue&queryarg2=testvalue2'),
    ])
def test_unescape_url(original_query, expected_query, stub_server, cryprox_worker_url, get_config):
    stub_server.set_handler(handler)
    proxied_data = requests.get(urljoin(cryprox_worker_url, 'test.js?{}'.format(original_query)),
                                headers={"host": TEST_LOCAL_HOST,
                                         system_config.PARTNER_TOKEN_HEADER_NAME: get_config('test_local').PARTNER_TOKENS[0]}).text

    assert proxied_data == expected_query


@pytest.mark.parametrize('url', [
    'https://static-mon.yandex.net/static/main.js?pid=test',
    'https://static-mon.yandex.net/static/main.js',
    'https://static-mon.yandex.net/static/0/',
])
def test_remove_fragment(url, get_key_and_binurlprefix_from_config, get_config):
    test_config = get_config('test_local')
    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)
    crypted_link = crypt_url(binurlprefix, '{}#{}'.format(url, 'fragment'), key, True, origin='test.local')
    response = requests.get(crypted_link,
                            headers={"host": TEST_LOCAL_HOST, system_config.PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]})

    processed_url = response.headers.get('X-Accel-Redirect')
    assert processed_url.find('#fragment') < 0


def test_remove_fragment_after_crypted(get_key_and_binurlprefix_from_config, get_config):
    test_config = get_config('test_local')
    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)
    crypted_link = crypt_url(binurlprefix, 'https://static-mon.yandex.net/static/0/', key, True, origin='test.local')
    response = requests.get('{}#{}'.format(crypted_link, 'fragment'),
                            headers={"host": TEST_LOCAL_HOST, system_config.PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]})

    processed_url = response.headers.get('X-Accel-Redirect')
    assert processed_url.find('#fragment') < 0


@pytest.mark.parametrize('domain, path, additional_query, expected_path, expected_query, expected_code', [
    ("aabturbo.gq", "bla-bla.js", "", "/bla-bla.js", "", 200),
    ("aabturbo.gq", "bla-bla.js?ver=5.2.4", "", "/bla-bla.js", "ver=5.2.4", 200),
    ("aabturbo.gq", "bla-bla.js", "ver=5.2.4", "/bla-bla.js", "ver=5.2.4", 200),
    ("aabturbo.gq", "bla-bla.js?ver=5.2.4", "param=1", "/bla-bla.js", "param=1&ver=5.2.4", 200),
    ("aabturbo.gq", "path/${replace}/bla-bla.js?ver=5.2.4", "param=1&replace=new_path", "/path/new_path/bla-bla.js", "param=1&ver=5.2.4", 200),
    ("aabturbo.gq", "bla-bla.js?param3=3%26param4=4", "param1=1&param2=2", "/bla-bla.js", "param1=1&param2=2&param3=3&param4=4", 200),
    ("", "", "", "/", "", 400),
    ("aabturbo.gq", "", "", "/", "", 400),
    ("", "bla-bla.js", "", "/bla-bla.js", "", 400),
    ("aabturbo.gq", "path/${replace}/bla-bla.js?ver=5.2.4", "param=1", "/path/new_path/bla-bla.js", "param=1&ver=5.2.4", 400),
])
def test_fixup_url_auredirect(domain, path, additional_query, expected_path, expected_query, expected_code,
                              stub_server, get_key_and_binurlprefix_from_config, cryprox_worker_address, get_config):

    def handler(**request):
        if request.get('path', '/') == expected_path and request.get('query', '') == expected_query:
            return {'text': 'var a,b,c;', 'code': 200, 'headers': {'Content-Type': 'text/javascript'}}
        return {'text': 'Unexpected request', 'code': 404}

    stub_server.set_handler(handler)

    key, binurlprefix = get_key_and_binurlprefix_from_config('autoredirect.turbo')
    crypted_url = crypt_url(binurlprefix, 'http://turbo.local/detect?domain={}&path={}&proto=http'.format(domain, path), key, False, origin='test.local')
    crypted_url = urlparse(crypted_url)._replace(netloc=cryprox_worker_address).geturl()
    crypted_url += "?{}".format(additional_query) if additional_query else ""
    proxied = requests.get(crypted_url, headers={"host": "aabturbo-gq{}".format(DEFAULT_COOKIELESS_HOST)})

    assert expected_code == proxied.status_code


@pytest.mark.parametrize('original_path,expected_path', [
    ['/appcry', '/changed/path'],
    ['/appcry/', '/changed/path'],
    ['/appcry/old/path', '/appcry/old/path'],
    ['/old/path', '/old/path'],
])
def test_change_fetch_url_from_header(stub_server, cryprox_worker_url, get_config, original_path, expected_path):

    def handler(**request):
        return {'text': request.get('path', ''), 'code': 200}

    stub_server.set_handler(handler)

    test_config = get_config('test_local')
    headers = {
        "host": TEST_LOCAL_HOST,
        system_config.PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0],
        system_config.FETCH_URL_HEADER_NAME: 'http://an.yandex.ru/changed/path',
    }

    response = requests.get(urljoin(cryprox_worker_url, original_path), headers=headers)
    assert response.text == expected_path


@pytest.mark.parametrize('request_url,expected_url', [
    ('http://yandex.ru/an/count/aaa', 'http://an.yandex.ru/count/aaa'),
    ('http://yandex.ua/an/count/aaa', 'http://an.yandex.ru/count/aaa'),
    ('http://yandex.com.am/an/count/aaa', 'http://an.yandex.ru/count/aaa'),
    ('http://yandex.ru/an/kakoi-to-path/file', 'http://an.yandex.ru/kakoi-to-path/file'),
    ('http://yandex.ru/ads/vmap/', 'http://yandex.ru/ads/vmap/'),
    ('http://yandex.ru/ads/page/smth', 'http://yandex.ru/ads/page/smth'),
])
def test_proxy_yandex_ru_an_to_an_yandex(stub_server, cryprox_worker_url, get_config, get_key_and_binurlprefix_from_config, request_url, expected_url):
    def handler(**request):
        proto = 'http://'
        host = request.get('headers', dict()).get('HTTP_HOST', '')
        path = request.get('path', '/')
        return {'text': proto + host + path, 'code': 200}

    stub_server.set_handler(handler)

    test_config = get_config('test_local')
    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)
    crypted_url = crypt_url(binurlprefix, request_url, key, True)
    response = requests.get(crypted_url, headers={'host': TEST_LOCAL_HOST,
                                                   system_config.PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]})

    assert response.text == expected_url


def test_adfox_loader_rewrite_url(stub_server, get_config, get_key_and_binurlprefix_from_config):
    def handler(**request):
        if request.get('path', '/') == '/system/context_adb.js':
            return {'text': 'successfully', 'code': 200}
        return {'code': 404, 'text': ''}

    test_config = get_config('test_local')

    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)

    stub_server.set_handler(handler)

    adfox_loader_url = "http://yastatic.net/pcode/adfox/loader_adb.js"
    crypted_link = crypt_url(binurlprefix, adfox_loader_url, key)
    headers = {'host': TEST_LOCAL_HOST, system_config.PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]}
    response = requests.get(crypted_link, headers=headers)
    assert response.headers.get(system_config.NGINX_SERVICE_ID_HEADER) == 'test_local'
    assert response.status_code == 200
    assert response.text == 'successfully'
