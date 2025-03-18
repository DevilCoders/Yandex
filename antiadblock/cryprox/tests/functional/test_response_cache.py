import pytest
import requests

from antiadblock.cryprox.cryprox.common.cryptobody import crypt_url
from antiadblock.cryprox.cryprox.config.system import PARTNER_TOKEN_HEADER_NAME, SEED_HEADER_NAME

CRYPTABLE_CONTENT = 'content_and_regexp_simultaneously'
NOT_CRYPTABLE_CONTENT = 'not_cryptable'
CRYPTED_CONTENT = 'crypted'
UNIQUE_HEADER = 'X-Unique-Header-To-Test-Headers-Are-Cached'
UNIQUE_HEADER_VALUE = 'value'


def handler_expected_to_be_cached(**request):
    path = request.get('path', '/')
    if path.endswith('http_code_404.js'):
        return {'text': CRYPTABLE_CONTENT, 'code': 404, 'headers': {'Content-Type': 'text/plain', UNIQUE_HEADER: UNIQUE_HEADER_VALUE}}
    elif path.endswith('http_code_502.js'):
        return {'text': CRYPTABLE_CONTENT, 'code': 502, 'headers': {'Content-Type': 'text/plain', UNIQUE_HEADER: UNIQUE_HEADER_VALUE}}
    elif path == '/test_no_cache_headers/max-age/public/___.js':
        return {'text': CRYPTABLE_CONTENT, 'code': 200,
                'headers': {'Content-Type': 'text/plain', UNIQUE_HEADER: UNIQUE_HEADER_VALUE, 'cache-control': 'max-age=604800, public'}}
    elif path == '/test_no_cache_headers/max-age/private/___.js':
        return {'text': CRYPTABLE_CONTENT, 'code': 200,
                'headers': {'Content-Type': 'text/plain', UNIQUE_HEADER: UNIQUE_HEADER_VALUE, 'cache-control': 'max-age=604800, private'}}
    elif path == '/test_no_cache_headers/no-store/___.js':
        return {'text': CRYPTABLE_CONTENT, 'code': 200,
                'headers': {'Content-Type': 'text/plain', UNIQUE_HEADER: UNIQUE_HEADER_VALUE, 'cache-control': 'max-age=604800, public, no-cache, no-store'}}
    else:
        return {'text': CRYPTABLE_CONTENT, 'code': 200, 'headers': {'Content-Type': 'text/plain', UNIQUE_HEADER: UNIQUE_HEADER_VALUE}}


def handler_with_another_response(**_):
    return {'text': NOT_CRYPTABLE_CONTENT, 'code': 200, 'headers': {'Content-Type': 'text/plain'}}


@pytest.mark.parametrize('url, use_cache_on, should_be_cached', [
    ('http://test.local/lib1.js', True, True),
    ('http://test.local/http_code_404.js', True, False),
    ('http://test.local/http_code_502.js', True, False),
    ('http://test.local/style.css', True, True),
    ('http://test.local/lib2.js?q1=1&q2=2', True, True),
    ('http://test.local/lib3.js', False, False),
    ('http://test.local/page.html', True, False),
])
def test_use_response_cache(stub_server, url, use_cache_on, should_be_cached, set_handler_with_config, get_config, get_key_and_binurlprefix_from_config):
    test_config = get_config('test_local')
    new_test_config = test_config.to_dict()
    new_test_config['REPLACE_BODY_RE'] = {CRYPTABLE_CONTENT: CRYPTED_CONTENT}
    new_test_config['USE_CACHE'] = use_cache_on
    set_handler_with_config(test_config.name, new_test_config)

    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)
    crypted_link = crypt_url(binurlprefix, url, key, False)

    stub_server.set_handler(handler_expected_to_be_cached)
    proxied = requests.get(crypted_link, headers={"host": 'test.local', PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]})

    if proxied.status_code in (200, 404):
        assert proxied.text == CRYPTED_CONTENT
    else:
        assert proxied.text == CRYPTABLE_CONTENT
    assert proxied.headers.get(UNIQUE_HEADER) == UNIQUE_HEADER_VALUE

    stub_server.set_handler(handler_with_another_response)
    proxied = requests.get(crypted_link, headers={"host": 'test.local', PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]})
    proxied_new_seed = requests.get(crypted_link, headers={"host": 'test.local', PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0], SEED_HEADER_NAME: 'other'})

    if should_be_cached:
        assert proxied.text == CRYPTED_CONTENT
        assert proxied.headers.get(UNIQUE_HEADER) == UNIQUE_HEADER_VALUE
    else:
        assert proxied.text == NOT_CRYPTABLE_CONTENT
        assert proxied.headers.get(UNIQUE_HEADER) is None

    assert proxied_new_seed.text == NOT_CRYPTABLE_CONTENT
    assert proxied_new_seed.headers.get(UNIQUE_HEADER) is None


@pytest.mark.parametrize('url, should_be_cached', [
    ('http://test.local/lib1.js', True),
    ('http://test.local/test_no_cache_headers/max-age/public/___.js', True),
    ('http://test.local/test_no_cache_headers/max-age/private/___.js', False),
    ('http://test.local/test_no_cache_headers/no-store/___.js', False),
])
def test_cache_with_cache_control_headers(stub_server, url, should_be_cached, set_handler_with_config, get_config, get_key_and_binurlprefix_from_config):
    test_config = get_config('test_local')
    new_test_config = test_config.to_dict()
    new_test_config['REPLACE_BODY_RE'] = {CRYPTABLE_CONTENT: CRYPTED_CONTENT}
    new_test_config['USE_CACHE'] = True
    set_handler_with_config(test_config.name, new_test_config)

    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)
    crypted_link = crypt_url(binurlprefix, url, key, False)

    stub_server.set_handler(handler_expected_to_be_cached)
    proxied = requests.get(crypted_link, headers={"host": 'test.local', PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]})

    assert proxied.text == CRYPTED_CONTENT
    assert proxied.headers.get(UNIQUE_HEADER) == UNIQUE_HEADER_VALUE

    stub_server.set_handler(handler_with_another_response)
    proxied = requests.get(crypted_link, headers={"host": 'test.local', PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]})

    if should_be_cached:
        assert proxied.text == CRYPTED_CONTENT
        assert proxied.headers.get(UNIQUE_HEADER) == UNIQUE_HEADER_VALUE
    else:
        assert proxied.text == NOT_CRYPTABLE_CONTENT
        assert proxied.headers.get(UNIQUE_HEADER) is None


@pytest.mark.parametrize('urls, no_cache_url_re, should_be_cached', [
    (['http://test.local/code.js', 'http://test.local/styles.css'], None, True),
    (['http://test.local/code.js', 'http://test.local/styles.css', 'http://aab-pub.s3.yandex.net/lib.browser.min.js'], ['test\.local/code\.js', 'test\.local/styles\.css'], False),
])
def test_no_cache_re(stub_server, urls, no_cache_url_re, should_be_cached, set_handler_with_config, get_config, get_key_and_binurlprefix_from_config):
    test_config = get_config('test_local')
    new_test_config = test_config.to_dict()
    new_test_config['REPLACE_BODY_RE'] = {CRYPTABLE_CONTENT: CRYPTED_CONTENT}
    new_test_config['USE_CACHE'] = True
    new_test_config['NO_CACHE_URL_RE'] = no_cache_url_re
    set_handler_with_config(test_config.name, new_test_config)

    for url in urls:
        key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)
        crypted_link = crypt_url(binurlprefix, url, key, False)

        stub_server.set_handler(handler_expected_to_be_cached)
        proxied = requests.get(crypted_link, headers={"host": 'test.local', PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]})

        assert proxied.text == CRYPTED_CONTENT
        assert proxied.headers.get(UNIQUE_HEADER) == UNIQUE_HEADER_VALUE

        stub_server.set_handler(handler_with_another_response)
        proxied = requests.get(crypted_link, headers={"host": 'test.local', PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]})

        if should_be_cached:
            assert proxied.text == CRYPTED_CONTENT
            assert proxied.headers.get(UNIQUE_HEADER) == UNIQUE_HEADER_VALUE
        else:
            assert proxied.text == NOT_CRYPTABLE_CONTENT
            assert proxied.headers.get(UNIQUE_HEADER) is None


def test_updated_headers_caching(stub_server, set_handler_with_config, get_key_and_binurlprefix_from_config, get_config):
    content = 'lorem ipsum'
    special_header = 'X-Special-Header'
    special_header_value = '1337'
    special_header_rewrite = '42'
    url = 'http://test.local/special_code.js'

    test_config = get_config('test_local')
    new_test_config = test_config.to_dict()
    new_test_config['UPDATE_RESPONSE_HEADERS_VALUES'] = {r'test\.local/.*': {special_header: special_header_rewrite}}
    new_test_config['USE_CACHE'] = True
    set_handler_with_config(test_config.name, new_test_config)

    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)
    crypted_link = crypt_url(binurlprefix, url, key, False)

    stub_server.set_handler(
        lambda **req: {'text': content, 'code': 200,
                       'headers': {'Content-Type': 'text/plain', special_header: special_header_value}}
    )
    proxied = requests.get(crypted_link,
                           headers={'host': 'test.local', PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]})

    assert proxied.text == content
    assert proxied.headers.get(special_header) == special_header_rewrite

    stub_server.set_handler(
        lambda **req: {'text': 'another one', 'code': 200, 'headers': {'Content-Type': 'text/plain'}})
    cached = requests.get(crypted_link,
                          headers={'host': 'test.local', PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]})

    assert cached.text == content
    assert cached.headers.get(special_header) == special_header_rewrite


HTTP_CONTENT = 'http'
HTTPS_CONTENT = 'https'


def test_forward_proto(stub_server, set_handler_with_config, get_config, get_key_and_binurlprefix_from_config):
    url = 'http://test.local/something.js'

    test_config = get_config('test_local')
    new_test_config = test_config.to_dict()
    new_test_config['USE_CACHE'] = True
    set_handler_with_config(test_config.name, new_test_config)

    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)
    crypted_link = crypt_url(binurlprefix, url, key, False)

    stub_server.set_handler(lambda **req: {'text': HTTP_CONTENT, 'code': 200, 'headers': {'Content-Type': 'text/plain'}})
    proxied = requests.get(
        crypted_link,
        headers={
            'host': 'test.local',
            PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0],
            'X-Forwarded-Proto': 'http'
        }
    )

    assert proxied.text == HTTP_CONTENT

    stub_server.set_handler(lambda **req: {'text': HTTPS_CONTENT, 'code': 200, 'headers': {'Content-Type': 'text/plain'}})
    proxied = requests.get(
        crypted_link,
        headers={
            'host': 'test.local',
            PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0],
            'X-Forwarded-Proto': 'http'
        }
    )

    assert proxied.text == HTTP_CONTENT

    proxied = requests.get(
        crypted_link,
        headers={
            'host': 'test.local',
            PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0],
            'X-Forwarded-Proto': 'https'
        }
    )

    assert proxied.text == HTTPS_CONTENT
