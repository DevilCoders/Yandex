# coding=utf-8
import pytest
from urlparse import urlparse

from antiadblock.libs.decrypt_url.lib import decrypt_url
from antiadblock.cryprox.cryprox.common.cryptobody import crypt_url, crypt_number
from .conftest import TEST_CRYPT_SECRET_KEY


@pytest.mark.parametrize('url', (
    'https://awaps.yandex.net/YkZbKgdWgD.png',
    '//an.yandex.ru/resource/banner.gif',
    'http://some.other.domain/??sldkfjsld',
    'https://direct.yandex.ru/?partner',
))
@pytest.mark.parametrize('url_appendage', ('', '?good+future+dynamic', '?good+future/dynamic'))
@pytest.mark.parametrize('min_length', (0, 40, 150))  # 40 - less then our crypted part
def test_crypt_decrypt_url_with_extended_symbols(url, url_appendage, get_test_key_and_binurlprefix, min_length):

    # generate random seed
    key, binurlprefix = get_test_key_and_binurlprefix(seed=None)
    crypted_url = crypt_url(binurlprefix, url, key, enable_trailing_slash=False, min_length=min_length)
    crypted_url = crypted_url + url_appendage

    decrypted_url, seed, origin = decrypt_url(urlparse(crypted_url)._replace(scheme='', netloc='').geturl(), str(TEST_CRYPT_SECRET_KEY), '/', False)

    assert len(crypted_url[len(binurlprefix.crypt_prefix(urlparse(url).scheme)):]) > min_length
    assert origin == binurlprefix.common_crypt_prefix.netloc
    assert url + url_appendage != crypted_url
    assert url + url_appendage == decrypted_url


@pytest.mark.parametrize('cryptable_url_part, url_appendage, expected_url', (
    ('https://domain.do', '?param', 'https://domain.do?param'),
    ('https://domain.do/', '?param', 'https://domain.do/?param'),
    ('https://domain.do/?param', '', 'https://domain.do/?param'),
    ('https://domain.do?v=value', '?param', 'https://domain.do?v=value&param'),
    ('https://domain.do/sub/path?v=value', '?param', 'https://domain.do/sub/path?v=value&param'),
    ('https://domain.do', '/param', 'https://domain.do/param'),
    ('https://domain.do', '/param?para', 'https://domain.do/param?para'),
    ('https://domain.do/some/path', '/param?para', 'https://domain.do/some/path/param?para'),
    ('//domain.do/some??parameter=21', '?other', '//domain.do/some??parameter=21&other'),
    ('//domain.do/path?param=foo', '&&lkdfjsldk', '//domain.do/path?param=foo&&lkdfjsldk'),
    ('//domain.do', '&&lkdfjsldk', '//domain.do?&&lkdfjsldk'),
    ('//domain.do', '&lkdfjsldk', '//domain.do?&lkdfjsldk'),
    ('http://some.other.domain/??qwerty', '?good+future/dynamic', 'http://some.other.domain/??qwerty?good+future/dynamic'),
))
def test_crypt_decrypt_url_special_cases(get_test_key_and_binurlprefix, cryptable_url_part, url_appendage, expected_url):

    # generate random seed
    key, binurlprefix = get_test_key_and_binurlprefix(seed=None)

    min_length = 150

    crypted_url = crypt_url(
        binurlprefix,
        cryptable_url_part,
        key,
        enable_trailing_slash=False,
        min_length=min_length,
    )
    crypted_url = crypted_url + url_appendage

    decrypted_url, seed, origin = decrypt_url(urlparse(crypted_url)._replace(scheme='', netloc='').geturl(), str(TEST_CRYPT_SECRET_KEY), '/', False)

    assert origin == binurlprefix.common_crypt_prefix.netloc
    assert expected_url != crypted_url
    assert expected_url == decrypted_url


@pytest.mark.parametrize('appendix', ['', '?mussor', '?querypar=1', '/querypar?querypar=yes', '/querypar?querypar=yes&&json=1'])
@pytest.mark.parametrize('min_length, with_trailing_slash', [
    (0, False),
    (0, True),
    (50, False),
    (50, True),
])
def test_decrypt_url_interface(appendix, min_length, with_trailing_slash, get_test_key_and_binurlprefix):
    url = 'https://awaps.yandex.net/YkZbKgdWgD.png'
    # generate random seed
    key, binurlprefix = get_test_key_and_binurlprefix(seed=None)

    crypted_url = crypt_url(binurlprefix, url, key, enable_trailing_slash=with_trailing_slash, min_length=min_length)
    crypted_url = crypted_url + appendix

    decrypted_url, seed, origin = decrypt_url(urlparse(crypted_url)._replace(scheme='', netloc='').geturl(), str(TEST_CRYPT_SECRET_KEY), '/', with_trailing_slash)

    assert seed == binurlprefix.seed
    assert url + appendix != crypted_url
    assert url + appendix == decrypted_url
    assert origin == binurlprefix.common_crypt_prefix.netloc


@pytest.mark.parametrize('number,seed,max_length,expected_result', [(42, 'my2007', 6, 'SN4633'),
                                                                    (42, 'creapy', 6, 'd4E279'),
                                                                    (42, 'my2007', 5, 'N4633'),
                                                                    (18, 'my2007', 6, 'SN2017'),
                                                                    (42, 'my2007', 60, 'zycXEhatMbCNswJKsQkUDrzAFZnCbzHVXahKwlIbjJpjVkkwXSWdt46XSN33')])
def test_crypt_number(number, seed, max_length, expected_result):

    crypted_number = crypt_number(number, seed, max_length)
    assert len(crypted_number) == max_length
    # Проверяем, что результат всегда один и тот же при неизменном seed
    for x in xrange(5):
        assert expected_result == crypted_number


@pytest.mark.parametrize('url', ('//ololololol.olo/path/',
                                 '//ololololol.olo/path/?param=value',
                                 '//ololololol.olo/path/#fragment'))
def test_add_trailing_slash_for_folder(url, get_test_key_and_binurlprefix):
    key, binurlprefix = get_test_key_and_binurlprefix(host='localhost')
    assert crypt_url(binurlprefix, url, key, enable_trailing_slash=False).endswith('/')


@pytest.mark.parametrize('test_seeds', (['my2007'],
                                        ['my2007', '12345'],
                                        ['my2007', '12345', 'AbCdE']))
def test_insert_slash_position(test_seeds, get_test_key_and_binurlprefix):
    """
    Test that slash positions are unchanged while crypting with the same seed
    """
    url = 'http://sometest.net/images/uprls.png'
    crypted_urls = []

    for test_seed in test_seeds:
        key, binurlprefix = get_test_key_and_binurlprefix(seed=test_seed)

        for x in xrange(1, 1000):
            crypted_urls.append(crypt_url(binurlprefix, url, key, False))

    assert len(set(crypted_urls)) == len(test_seeds) and len(crypted_urls) == 999 * len(test_seeds)


@pytest.mark.parametrize('url', ('https://awaps.yandex.net/YkZbKgdWgD.png', '//an.yandex.ru/resource/banner.gif'))
@pytest.mark.parametrize('prefix,can_decrypt', (('/', True),
                                                ('/prefix1/', True),
                                                ('/prefix2/', True),
                                                ('/prefix3/', False)))
def test_crypt_decrypt_url_with_prefix(url, get_test_key_and_binurlprefix, prefix, can_decrypt):
    # generate random seed
    host = 'test.local'
    key, binurlprefix = get_test_key_and_binurlprefix(seed=None, host=host, crypt_url_prefix=prefix)
    prefixes = '(?:{})'.format('|'.join(['/', '/prefix1/', '/prefix2/']))
    crypted_url = crypt_url(binurlprefix, url, key)

    assert url != crypted_url

    decrypted_url, seed, origin = decrypt_url(urlparse(crypted_url)._replace(scheme='', netloc='').geturl(),
                                              str(TEST_CRYPT_SECRET_KEY), str(prefixes), False)

    if can_decrypt:
        assert url == decrypted_url
    else:
        assert decrypted_url is None
