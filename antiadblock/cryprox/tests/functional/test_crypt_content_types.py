import cgi

import pytest
import requests

from antiadblock.cryprox.cryprox.common.cryptobody import crypt_url, CryptUrlPrefix
from antiadblock.cryprox.cryprox.config.system import SEED_HEADER_NAME, PARTNER_TOKEN_HEADER_NAME
from antiadblock.cryprox.tests.lib.containers_context import TEST_LOCAL_HOST

crypted_mime_types = (
    'text/html',
    'text/css',
    'text/plain',
    'text/xml',
    'application/javascript',
    'application/json',
    'application/x-javascript',
    'text/javascript')


@pytest.mark.parametrize('mime_type, is_crypted', [(mime, True) for mime in crypted_mime_types] +
                         [(mime, False) for mime in ('application/pdf', 'application/xml', 'text/php')])
def test_crypted_file_mimetype(stub_server, mime_type, is_crypted, get_key_and_binurlprefix_from_config, get_config):

    test_config = get_config('test_local')
    binurlprefix_expected = CryptUrlPrefix('http', TEST_LOCAL_HOST, 'my2007', '/')
    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)

    test_url_to_crypt = '//avatars.mds.yandex.net/get-canvas/26126/catalog.3231.775214863238148215/logo'
    crypted_url = crypt_url(binurlprefix_expected, test_url_to_crypt, key, True, origin='test.local')

    raw_data = "<href='{}' />".format(test_url_to_crypt)
    expected_crypted_data = "<href='{}' />".format(crypted_url)

    def handler(**request):
        if request.get('path', '/') == '/mimetypes':
            return {'code': 200,
                    'text': raw_data,
                    'headers': {'Content-Type': request.get('headers', {}).get('HTTP_MIME')}}
        return {}

    stub_server.set_handler(handler)

    crypted_link = crypt_url(binurlprefix, 'http://{}/mimetypes'.format(TEST_LOCAL_HOST), key, True, origin='test.local')
    response = requests.get(crypted_link, headers={'host': TEST_LOCAL_HOST, 'mime': mime_type,
                                                   SEED_HEADER_NAME: 'my2007',
                                                   PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]})

    assert (expected_crypted_data if is_crypted else raw_data) == response.text
    assert len(response.headers['Content-Type'].split(',')) == 1
    assert cgi.parse_header(response.headers.get('Content-Type'))[0] == mime_type


@pytest.mark.parametrize('file, mime_type, expected_mime_type', [
    ('mimetypes.js', '', 'application/javascript'),
    ('mimetypes.js', 'text/html', 'application/javascript'),
    ('mimetypes.js', 'text/plain', 'application/javascript'),
    ('mimetypes.js', 'application/javascript', 'application/javascript'),
    ('mimetypes.js', 'application/json', 'application/json'),
    ('mimetypes.js', 'text/javascript', 'text/javascript'),
    ('mimetypes.css', '', 'text/css'),
    ('mimetypes.css', 'text/html', 'text/css'),
    ('mimetypes.css', 'text/plain', 'text/css'),
    ('mimetypes.css', 'text/css', 'text/css'),
    ('mimetypes.html', 'text/html', 'text/html'),
])
def test_fixed_mimetype(file, mime_type, expected_mime_type, stub_server, get_key_and_binurlprefix_from_config, get_config):

    def handler(**_):
        return {'code': 200,
                'text': '',
                'headers': {'Content-Type': mime_type}}

    stub_server.set_handler(handler)
    test_config = get_config('test_local')
    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)

    crypted_link = crypt_url(binurlprefix, 'http://{}/{}'.format(TEST_LOCAL_HOST, file), key, True, origin='test.local')
    response = requests.get(crypted_link, headers={'host': TEST_LOCAL_HOST,
                                                   SEED_HEADER_NAME: 'my2007',
                                                   PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]})

    assert response.status_code == 200
    assert len(response.headers['Content-Type'].split(',')) == 1
    assert cgi.parse_header(response.headers.get('Content-Type'))[0] == expected_mime_type
