# -*- coding: utf8 -*-
import pytest
import requests

from antiadblock.cryprox.cryprox.common.cryptobody import crypt_url
from antiadblock.cryprox.cryprox.config import system as system_config
from antiadblock.cryprox.tests.lib.containers_context import TEST_LOCAL_HOST
from antiadblock.cryprox.tests.lib.an_yandex_utils import expand_an_yandex


@pytest.mark.parametrize('url_base, query, test_tag, expected_test_tag', []
    # ссылки к которым cryprox должна добалять test-tag с нужным битом
    + expand_an_yandex('/an', 'http://an.yandex.ru/count/', 'query1=1&query2=2', 77777777, 77777777 | 1 << 49)
    + expand_an_yandex('/an', 'http://an.yandex.ru/count/', '', '', 1 | 1 << 49)
    + expand_an_yandex('/an', 'http://an.yandex.ru/count/', '', '0', 1 << 49)
    + expand_an_yandex('/an', 'http://an.yandex.ru/count/', '', 123 | 1 << 49, 123 | 1 << 49)
    + [('http://bs.yandex.ru/count/', '', '', 1 | 1 << 49)]
    # ссылки, test-tag у которых меняться не должен
    + expand_an_yandex('/ads', 'http://an.yandex.ru/meta/', 'aadb=2', 77777777, 77777777)
    + expand_an_yandex('/ads', 'http://an.yandex.ru/meta/', '', '', '')
    + [
    ('http://bs.yandex.ru/meta/', '', '', ''),
    ('http://yabs.yandex.ru/meta/', '', '', ''),
    ('http://ads.adfox.ru/123/getCode/', '', 123, 123),
])
def test_rtb_counters_test_tag(url_base, query, expected_test_tag, test_tag, stub_server, get_config, get_key_and_binurlprefix_from_config):
    """
    Проверяет корректное изменение антиадблочного бита в тесттагах у ссылок на счетчики, и то что в остальных ссылках test-tag не меняется.
    :param url_base
    :param query
    :param add_tag должна ли cryprox изменять 50-й бит
    :param test_tag значение test-tag в url запроса, '' - значит параметр отсутствует
    """
    def handler(**request):
        headers = {'x-aab-uri': request['path'] + '?' + request.get('query', '')}
        return {'text': 'ok', 'code': 200, 'headers': headers}

    stub_server.set_handler(handler)
    test_config = get_config('test_local')
    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)

    query += '&test-tag={}'.format(test_tag) if test_tag else ''
    url = url_base + '?' + query if query else url_base
    crypted_link = crypt_url(binurlprefix, url, key, True)
    response = requests.get(crypted_link, headers={'host': TEST_LOCAL_HOST, system_config.PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]})
    if not expected_test_tag:
        assert response.headers['x-aab-uri'].count('test-tag') == 0
    else:
        assert response.headers['x-aab-uri'].count('test-tag') == 1
        assert 'test-tag={}'.format(expected_test_tag) in response.headers['x-aab-uri']


@pytest.mark.parametrize('url_base, query, adb_bits, expected_adb_bits', []
    + expand_an_yandex('/an', 'http://an.yandex.ru/count/', 'query1=1&query2=2', 2, 2 | 1)
    + expand_an_yandex('/an', 'http://an.yandex.ru/count/', '', '', 1)
    + expand_an_yandex('/an', 'http://an.yandex.ru/count/', '', '0', 1)
    + expand_an_yandex('/an', 'http://an.yandex.ru/count/', '', 2 | 1, 2 | 1)
    + [
    ('http://bs.yandex.ru/count/', '', '', 1),
    ('http://adsdk.yandex.ru:80/proto/report/1622/KYB=/71/an.yandex.ru/count/WP0ejI', '', '', 1)
    ]
    + expand_an_yandex('/ads', 'http://an.yandex.ru/meta/', 'aadb=2', 2, 2 | 1)
    + expand_an_yandex('/ads', 'http://an.yandex.ru/meta/', '', '', 1)
    + expand_an_yandex('/ads', 'http://an.yandex.ru/meta/', '', 1, 1)
    + expand_an_yandex('/ads', 'http://an.yandex.ru/vmap/', '', '', 1)
    + [
    ('http://bs.yandex.ru/meta/', '', '', 1),
    ('http://yabs.yandex.ru/meta/', '', '', 1),
    ('http://ads.adfox.ru/123/getCode/', '', 2, 2 | 1),
    ('http://adsdk.yandex.ru:80/proto/report/1622/KYB=/71/an.yandex.ru/abuse/WP0ejI', '', '', ''),
])
def test_rtb_counters_adb_bits(url_base, query, adb_bits, expected_adb_bits, stub_server, get_config,
                               get_key_and_binurlprefix_from_config):
    """
    Проверяет корректное изменение антиадблочного бита в adb-bits у ссылок на счетчики и аукционы
    :param url_base
    :param query
    :param adb_bits значение adb_bits в url запроса, '' - значит параметр отсутствует
    :param expected_adb_bits ожидаемый adb_bits в ответе
    """

    def handler(**request):
        headers = {'x-aab-uri': request['path'] + '?' + request.get('query', '')}
        return {'text': 'ok', 'code': 200, 'headers': headers}

    stub_server.set_handler(handler)
    test_config = get_config('test_local')
    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)

    query += '&adb-bits={}'.format(adb_bits) if adb_bits else ''
    url = url_base + '?' + query if query else url_base
    crypted_link = crypt_url(binurlprefix, url, key, True)
    response = requests.get(crypted_link, headers={'host': TEST_LOCAL_HOST,
                                                   system_config.PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]})
    if not expected_adb_bits:
        assert response.headers['x-aab-uri'].count('adb-bits') == 0
    else:
        assert response.headers['x-aab-uri'].count('adb-bits') == 1
        assert 'adb-bits={}'.format(expected_adb_bits) in response.headers['x-aab-uri']
