import pytest
import requests
from urlparse import urljoin, urlparse, parse_qsl

from antiadblock.libs.decrypt_url.lib import get_key
from antiadblock.cryprox.cryprox.common.cryptobody import crypt_url, CryptUrlPrefix
from antiadblock.cryprox.cryprox.common.cry import generate_seed
from antiadblock.cryprox.cryprox.config import system as system_config
from antiadblock.cryprox.cryprox.config.system import DEBUG_API_TOKEN, DEBUG_RESPONSE_HEADER_NAME, DEFAULT_URL_TEMPLATE, NGINX_SERVICE_ID_HEADER
from antiadblock.cryprox.tests.lib.constants import cryprox_error_message
from antiadblock.cryprox.tests.lib.containers_context import TEST_LOCAL_HOST


@pytest.fixture
def handler(cryprox_worker_url):

    def impl(**request):
        if '/my2007j7/mGf6SlKlElf/JpVUW-7/uo-ay-/Q_OsX/I-ed05g/GltLB6HnN8RRQ' in request.get('path', '/'):  # crypted 'woop-woop-test'
            return {'code': 404, 'text': 'accidentally decrypt'}
        elif '/accidentally_decrypt' in request.get('path', '/'):
            return {'code': 200, 'text': 'accidentally decrypt'}
        elif '/my20' in request.get('path', '/'):
            return {'redirect': cryprox_worker_url}
        elif '/pogoda/index.html' in request.get('path', '/'):
            return {'code': 200, 'text': '<!DOCTYPE html><html><body><h1>pogoda example</h1></body></html>', 'headers': {'Content-Type': 'text/html'}}
        else:
            query = request.get('query', '')
            return {'headers': {'x-aab-uri': request.get('path', '/') + ('?' + query if query else '')}}

    return impl


@pytest.fixture
def my_crypt_url(get_key_and_binurlprefix_from_config, request):
    key, binurlprefix = get_key_and_binurlprefix_from_config("test_local")

    def impl(url, suffix='', slice=None, enable_trailing_slash=False, crypted_url_mixing_template=DEFAULT_URL_TEMPLATE, x_forwarded_proto="http"):
        crypted_link = crypt_url(binurlprefix, url, key, enable_trailing_slash, crypted_url_mixing_template=crypted_url_mixing_template, origin='test.local')
        if not urlparse(crypted_link).scheme:
            return x_forwarded_proto + ":" + crypted_link + suffix
        if slice:
            return (crypted_link + suffix)[slice['start']:slice['stop']:slice['step']]
        return crypted_link + suffix

    # one can use fixture both ways: as callable or indirect parametrized
    return impl(**request.param) if hasattr(request, 'param') else impl


@pytest.mark.parametrize('my_crypt_url', ({"url": '//{}/test.bin?param1=value1&param2=value2&param3=%68%74%74%70%3A%2F%2Fya.ru'.format(TEST_LOCAL_HOST)},  # > 1 fake query param
                                          {"url": '//{}/'.format(TEST_LOCAL_HOST), "suffix": 'test.bin'},  # some situation from javascript work
                                          {"url": '//{}/pogoda'.format(TEST_LOCAL_HOST), "suffix": 'index.html', "enable_trailing_slash": True},  # another situation from javascript work
                                          {"url": '//{}/test.bin'.format(TEST_LOCAL_HOST)}),
                         indirect=True)
def test_decrypt_positive(stub_server, handler, my_crypt_url, get_config):
    stub_server.set_handler(handler)
    response = requests.get(my_crypt_url, headers={"host": TEST_LOCAL_HOST,
                                                   system_config.PARTNER_TOKEN_HEADER_NAME: get_config('test_local').PARTNER_TOKENS[0]})

    assert 200 == response.status_code


@pytest.mark.parametrize('my_crypt_url', (
    {"url": '', "slice": dict(start=0, stop=-1, step=1)},  # empty crypted part
    {"url": 'any text with spaces'},  # not an url
), indirect=True)
def test_decrypt_negative(stub_server, my_crypt_url, handler, get_config):
    stub_server.set_handler(handler)
    response = requests.get(my_crypt_url, headers={"host": TEST_LOCAL_HOST,
                                                   system_config.PARTNER_TOKEN_HEADER_NAME: get_config('test_local').PARTNER_TOKENS[0]})

    assert 403 == response.status_code
    assert cryprox_error_message == response.text


@pytest.mark.parametrize(('my_crypt_url', 'expected_code'), [(dict(url='woop-woop-test'), 404),  # see handler for hardcoded url
                                                             (dict(url='http://test.local/accidentally_decrypt'), 200)], indirect=['my_crypt_url'])
def test_accidentally_decrypt(stub_server, handler, expected_code, my_crypt_url, get_config):
    """
    Test if we decrypted path successfully but its not crypted url )
    :param my_crypt_url: url path to decrypt
    :param expected_code: expected http status code
    """
    stub_server.set_handler(handler)
    response = requests.get(my_crypt_url,
                            headers={"host": TEST_LOCAL_HOST,
                                     system_config.PARTNER_TOKEN_HEADER_NAME: get_config('test_local').PARTNER_TOKENS[0]})

    assert response.status_code == expected_code
    assert response.text == "accidentally decrypt"


@pytest.mark.usefixtures('stub_server')
def test_different_seeds(cryprox_worker_address, cryprox_worker_url, get_config):
    url_to_be_crypted = 'http://{}/index.html'.format(TEST_LOCAL_HOST)

    responses = []
    for seed in ['my2007', 'my2017']:
        config = get_config('test_local')
        binurlprefix = CryptUrlPrefix('http', cryprox_worker_address, seed, '/')
        secret_key = config.CRYPT_SECRET_KEY
        key = get_key(secret_key, seed)
        responses.append(requests.get(crypt_url(binurlprefix, url_to_be_crypted, key, True, origin='test.local'),
                                      headers={"host": TEST_LOCAL_HOST,
                                               system_config.PARTNER_TOKEN_HEADER_NAME: config.PARTNER_TOKENS[0]}))

    responses.append(requests.get(urljoin(cryprox_worker_url, 'index.html'),
                                  headers={"host": TEST_LOCAL_HOST,
                                           system_config.PARTNER_TOKEN_HEADER_NAME: config.PARTNER_TOKENS[0],
                                           system_config.SEED_HEADER_NAME: 'my2007'}))

    assert responses[0].text != responses[1].text
    assert responses[2].text == responses[0].text


@pytest.mark.usefixtures('stub_server')
def test_seed_decoded_long_url_fetch(cryprox_worker_url, get_config):
    """
    Test detecting seed when fetching decoded page with long url
    """
    test_config = get_config('test_local')
    test_url = urljoin(cryprox_worker_url, 'moskva/cars/index.html')
    seed = generate_seed(salt="test")
    responses = []

    responses.append(requests.get(test_url, headers={'host': TEST_LOCAL_HOST, system_config.PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]}))
    responses.append(requests.get(test_url, headers={'host': TEST_LOCAL_HOST, system_config.PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0],
                                                     system_config.SEED_HEADER_NAME: seed}))
    responses.append(requests.get(test_url, headers={'host': TEST_LOCAL_HOST, system_config.PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0],
                                                     system_config.SEED_HEADER_NAME: 'my2007'}))

    assert responses[0].text == responses[1].text
    assert responses[1].text != responses[2].text


@pytest.mark.usefixtures('stub_server')
def test_decrypt_with_slashes(my_crypt_url, get_config):
    url = my_crypt_url(url='//{}/test.bin'.format(TEST_LOCAL_HOST))
    response = requests.get(url, headers={"host": TEST_LOCAL_HOST,
                                          system_config.PARTNER_TOKEN_HEADER_NAME: get_config('test_local').PARTNER_TOKENS[0]})

    assert 200 == response.status_code


def test_decrypt_partially_encrypted_urls_with_slashes(stub_server, my_crypt_url, handler, get_config):
    stub_server.set_handler(handler)
    url_with_slashes = my_crypt_url('//{}/pogoda/'.format(TEST_LOCAL_HOST)) + 'index.html'
    response1 = requests.get(url_with_slashes, headers={"host": TEST_LOCAL_HOST, system_config.PARTNER_TOKEN_HEADER_NAME: get_config('test_local').PARTNER_TOKENS[0]})
    response_nginx = requests.get(urljoin(stub_server.url, '/pogoda/index.html'))
    assert response1.text == response_nginx.text


@pytest.mark.parametrize('my_crypt_url, endswith', [
    ({"url": '//{}/uri_echo/index.html'.format(TEST_LOCAL_HOST), "suffix": '?param=value'}, 'index.html?param=value'),
    ({"url": '//{}/uri_echo/index.html?param'.format(TEST_LOCAL_HOST), "suffix": '=value'}, 'index.html?param=value'),
    ({"url": '//{}/uri_echo/index.html?param='.format(TEST_LOCAL_HOST), "suffix": 'value'}, 'index.html?param=value'),
    ({"url": '//{}/uri_echo/index.html?param=value'.format(TEST_LOCAL_HOST), "suffix": '&param2=value2'}, 'index.html?param=value&param2=value2'),
    ({"url": '//{}/uri_echo'.format(TEST_LOCAL_HOST), "suffix": '/index.html?param=value'}, 'index.html?param=value'),
    ({"url": '//{}/uri_echo/index.html'.format(TEST_LOCAL_HOST), "suffix": '#1234567890'}, 'index.html'),
    ({"url": '//{}/uri_echo/?'.format(TEST_LOCAL_HOST), "suffix": 'param=value'}, '/?param=value'),
    ({"url": '//{}/uri_echo/'.format(TEST_LOCAL_HOST), "suffix": 'index.html'}, 'uri_echo/index.html'),
], indirect=['my_crypt_url'])
def test_decrypt_partially_encrypted_urls_with_query_string(stub_server, handler, my_crypt_url, endswith, get_config):
    """
    Often JS has concatenation like this 'http://domain.tld/path/?'+'param=value', but others may occur too
    """
    stub_server.set_handler(handler)
    response = requests.get(my_crypt_url, headers={"host": TEST_LOCAL_HOST,
                                                   system_config.PARTNER_TOKEN_HEADER_NAME: get_config('test_local').PARTNER_TOKENS[0]})
    x_aab_uri = response.headers['x-aab-uri']
    assert x_aab_uri.endswith(endswith)


# N is for test differentiation when fails
@pytest.mark.parametrize('url, crypted_url_mixing_template, x_forwarded_proto, scheme, N', [
    ('//{}/uri_echo/index.html'.format(TEST_LOCAL_HOST), DEFAULT_URL_TEMPLATE, 'http', '', 1),
    ('//{}/uri_echo/index.html'.format(TEST_LOCAL_HOST), (('/', 1), ('?', 1), ('=&', 10)), 'http', '', 2),
    ('//{}/uri_echo/index.html'.format(TEST_LOCAL_HOST), DEFAULT_URL_TEMPLATE, 'https', '', 3),
    ('//{}/uri_echo/index.html'.format(TEST_LOCAL_HOST), (('/', 1), ('?', 1), ('=&', 10)), 'https', '', 4),
    ('https://{}/uri_echo/index.html'.format(TEST_LOCAL_HOST), DEFAULT_URL_TEMPLATE, 'https', 'https', 5),
    ('https://{}/uri_echo/index.html'.format(TEST_LOCAL_HOST), (('/', 1), ('?', 1), ('=&', 10)), 'https', 'https', 6),
    ('https://{}/uri_echo/index.html'.format(TEST_LOCAL_HOST), DEFAULT_URL_TEMPLATE, 'http', 'https', 7),
    ('https://{}/uri_echo/index.html'.format(TEST_LOCAL_HOST), (('/', 1), ('?', 1), ('=&', 10)), 'http', 'https', 8),
    ('http://{}/uri_echo/index.html'.format(TEST_LOCAL_HOST), DEFAULT_URL_TEMPLATE, 'https', 'http', 9),
    ('http://{}/uri_echo/index.html'.format(TEST_LOCAL_HOST), (('/', 1), ('?', 1), ('=&', 10)), 'https', 'http', 10),
    ('http://{}/uri_echo/index.html'.format(TEST_LOCAL_HOST), DEFAULT_URL_TEMPLATE, 'http', 'http', 11),
    ('http://{}/uri_echo/index.html'.format(TEST_LOCAL_HOST), (('/', 1), ('?', 1), ('=&', 10)), 'http', 'http', 12),
])
def test_crypted_links_saves_scheme(my_crypt_url, get_config, url, crypted_url_mixing_template, x_forwarded_proto, scheme, N):
    """
    The schema of encrypted link doesn't matter. Always use the schema of inner link
    P.S.  didn't use stub server here, because stub server has no ssl
    """

    crypted_url = my_crypt_url(url=url,
                               enable_trailing_slash=False,
                               x_forwarded_proto="http",
                               crypted_url_mixing_template=crypted_url_mixing_template)
    response = requests.get(crypted_url, headers={"host": TEST_LOCAL_HOST,
                                                  system_config.PARTNER_TOKEN_HEADER_NAME: get_config('test_local').PARTNER_TOKENS[0],
                                                  "x-aab-debug-token": DEBUG_API_TOKEN,
                                                  "x-forwarded-proto": x_forwarded_proto})
    assert response.status_code == 200
    decrypted_url = response.headers[DEBUG_RESPONSE_HEADER_NAME]
    assert urlparse(decrypted_url).scheme == scheme


PATHS = [
    # Just random links with 5 and 6 first symbols
    '/33car/autoru/buy/olololo/gfhjbkgf', '/33cars/autoru/buy/olololo/gfhjbkgf',
    '/moscow/autorumobile/buy/?like=share', '/video/2392510-tjazheloatlet-gantelej.html',
    # ord(2) = 50, ord(d) = 100, num_of_slashes = 3
    # 50 * 3 + 100 = 250
    # This is url that looks like crypted url with slashes
    '/a2a50/2aaaad/a/b/c/d',
    '/a2a50a/2aaaad/a/b/c/d?param=value',
]


@pytest.mark.parametrize('path', PATHS)
def test_non_crypted_urls_with_slashes(stub_server, path, handler, cryprox_worker_url, get_config):
    stub_server.set_handler(handler)
    response = requests.get(urljoin(cryprox_worker_url, path),
                            headers={"host": TEST_LOCAL_HOST,
                                     system_config.PARTNER_TOKEN_HEADER_NAME: get_config('test_local').PARTNER_TOKENS[0]})
    assert response.status_code == 404
    assert response.headers['x-aab-uri'] == path


@pytest.mark.parametrize('enable_trailing_slash', (True, False))
@pytest.mark.usefixtures('stub_server')
def test_decrypt_mode(my_crypt_url, enable_trailing_slash, get_config):
    url = my_crypt_url('//{}/test.bin'.format(TEST_LOCAL_HOST),
                       enable_trailing_slash=enable_trailing_slash)
    response = requests.get(url, headers={
        'host': TEST_LOCAL_HOST,
        system_config.PARTNER_TOKEN_HEADER_NAME: get_config('test_local').PARTNER_TOKENS[0],
        system_config.DEBUG_TOKEN_HEADER_NAME: system_config.DEBUG_API_TOKEN,
    })
    decrypted_url = response.headers.get(system_config.DEBUG_RESPONSE_HEADER_NAME)
    assert response.headers.get(NGINX_SERVICE_ID_HEADER) == 'test_local'
    assert decrypted_url != url
    assert decrypted_url == '//{}/test.bin'.format(TEST_LOCAL_HOST)


@pytest.mark.usefixtures('stub_server')
def test_decrypt_mode_header_auth(my_crypt_url, get_config):
    url = my_crypt_url('//{}/test.bin'.format(TEST_LOCAL_HOST))
    test_config = get_config('test_local')
    for wrong_token in [system_config.DEBUG_API_TOKEN[1:], test_config.PARTNER_TOKENS[0]]:
        response = requests.get(url, headers={
            'host': TEST_LOCAL_HOST,
            system_config.PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0],
            # Request with wrong header value
            system_config.DEBUG_TOKEN_HEADER_NAME: wrong_token,
        })
        assert response.headers.get(NGINX_SERVICE_ID_HEADER) == 'test_local'
        assert response.status_code == 403
        assert response.text == 'Forbidden'


@pytest.mark.parametrize('preffix, expected_code', (("/preffix/", 404), ("/preffix1/", 200), ("/preffix2/", 200), ("/preffix3/", 404)))
def test_decrypt_url_with_old_preffix(set_handler_with_config, get_key_and_binurlprefix_from_config, get_config, preffix, expected_code):
    test_config = get_config('test_local')
    new_test_config = test_config.to_dict()
    new_test_config['CRYPT_URL_PREFFIX'] = "/preffix0/"
    new_test_config['CRYPT_URL_OLD_PREFFIXES'] = ['/', preffix] if expected_code == 200 else ['/']
    set_handler_with_config(test_config.name, new_test_config)

    key, binurlprefix = get_key_and_binurlprefix_from_config('test_local')
    crypted_url = crypt_url(binurlprefix, 'http://{}/test.bin'.format(TEST_LOCAL_HOST), key, False)
    response = requests.get(crypted_url, headers={
        "host": TEST_LOCAL_HOST,
        system_config.PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0],
    })

    assert response.status_code == 200
    crypted_url_old_preffix = crypted_url.replace("/preffix0/", preffix)
    response = requests.get(crypted_url_old_preffix, headers={
        "host": TEST_LOCAL_HOST,
        system_config.PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0],
    })
    assert response.status_code == expected_code


@pytest.mark.parametrize('random_preffixes', (
    ['/preffix1/', '/preffix2/', '/preffix3/', '/preffix4/', '/preffix5/', '/preffix6/'],
))
def test_decrypt_random_url_preffixes(set_handler_with_config, get_config, get_key_and_binurlprefix_from_config, random_preffixes):
    test_config = get_config('test_local')
    initial_preffix = "/prefffix0/"
    new_test_config = test_config.to_dict()
    new_test_config['CRYPT_URL_PREFFIX'] = initial_preffix
    new_test_config['CRYPT_URL_RANDOM_PREFFIXES'] = random_preffixes
    set_handler_with_config(test_config.name, new_test_config)

    key, binurlprefix = get_key_and_binurlprefix_from_config('test_local')
    crypted_url = crypt_url(binurlprefix, 'http://{}/test.bin'.format(TEST_LOCAL_HOST), key, False)
    response = requests.get(crypted_url, headers={
        "host": TEST_LOCAL_HOST,
        system_config.PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0],
    })

    assert response.status_code == 200

    for preffix in random_preffixes:
        crypted_url_random_preffix = crypted_url.replace(initial_preffix, preffix)
        response = requests.get(crypted_url_random_preffix, headers={
            "host": TEST_LOCAL_HOST,
            system_config.PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0],
        })
        assert response.status_code == 200
        initial_preffix = preffix


@pytest.mark.parametrize('rtb_auction_via_script', (False, True))
def test_decrypt_bk_meta_auction_url(stub_server, get_key_and_binurlprefix_from_config, get_config, set_handler_with_config,
                                     handler, rtb_auction_via_script):
    stub_server.set_handler(handler)

    partner_name = "test_local"
    test_config = get_config(partner_name)
    new_test_config = test_config.to_dict()
    new_test_config['RTB_AUCTION_VIA_SCRIPT'] = rtb_auction_via_script

    set_handler_with_config(test_config.name, new_test_config)

    key, binurlprefix = get_key_and_binurlprefix_from_config(partner_name)

    original_url = 'http://an.yandex.ru/meta/rtb_auction_sample?test=woah&callback=Ya%5B1524124928839%5D'
    crypted_url = crypt_url(binurlprefix, original_url, key, True)
    response = requests.get(crypted_url, headers={
        "host": TEST_LOCAL_HOST,
        system_config.PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0],
    })
    request_qargs = parse_qsl(response.headers['x-aab-uri'])
    assert ('callback', 'json') in request_qargs
    assert ('callback', 'Ya[1524124928839]') not in request_qargs
