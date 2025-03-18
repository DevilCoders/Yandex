# -*- coding: utf8 -*-
from urlparse import urljoin, urlparse

import json
import requests
import pytest

from antiadblock.libs.decrypt_url.lib import get_key, decrypt_url
from antiadblock.cryprox.cryprox.config.system import SEED_HEADER_NAME, PARTNER_TOKEN_HEADER_NAME, \
    CRYPTED_HOST_HEADER_NAME
from antiadblock.cryprox.cryprox.common.cryptobody import crypt_url, CryptUrlPrefix
from antiadblock.cryprox.tests.lib.containers_context import TEST_LOCAL_HOST


@pytest.mark.parametrize('crypted_host_header, crypted_host, host_header', [
    (None, None, TEST_LOCAL_HOST),
    (None, 'test.com', TEST_LOCAL_HOST),
    ('test.com', 'test.com', TEST_LOCAL_HOST),
    ('test.com', None, TEST_LOCAL_HOST),
    ('test.ru', 'test.com', TEST_LOCAL_HOST)])
def test_crypt_host_header(stub_server, cryprox_worker_url, set_handler_with_config, get_config, host_header, crypted_host, crypted_host_header):
    """
    Проверяем работу параметра CRYPTED_HOST из конфига партнера при шифровании урлов в обычном json-е
    :param host_header: http заголовок host
    :param crypted_host: значение параметра CRYPTED_HOST в конфиге партнера
    :param crypted_host_header: заголовок x-aab-crypted-host
    """

    # Подставляем crypted_host в параметр CRYPTED_HOST в конфиге партнера в заглушке config-api и рестартим ее
    test_config = get_config('test_local')
    new_test_config = test_config.to_dict()
    new_test_config['CRYPTED_HOST'] = crypted_host

    set_handler_with_config(test_config.name, new_test_config)

    seed = 'my2007'
    config = get_config('test_local')

    # Проверяем, что параметр подставился корректно
    if crypted_host:
        assert config.CRYPTED_HOST == crypted_host
    else:
        assert config.CRYPTED_HOST is None

    key = get_key(config.CRYPT_SECRET_KEY, seed)
    host = crypted_host_header or crypted_host or host_header
    binurlprefix = CryptUrlPrefix('http', host, seed, config.CRYPT_URL_PREFFIX)

    original_json = requests.get(urljoin(stub_server.url, '/simple_json.html')).json()
    crypted_json = requests.get(urljoin(cryprox_worker_url, '/simple_json.html'),
                                headers={"host": host_header,
                                         SEED_HEADER_NAME: seed,
                                         CRYPTED_HOST_HEADER_NAME: crypted_host_header,
                                         PARTNER_TOKEN_HEADER_NAME: config.PARTNER_TOKENS[0]}).json()

    # Проверяем, что урлы, которые не должны шифроваться, не пошифровались
    assert [y for x, y in original_json.items() if x.startswith('not_crypted_url')] == [y for x, y in crypted_json.items() if x.startswith('not_crypted_url')]

    crypted_links = [y for x, y in crypted_json.items() if x.startswith('crypted_url')]

    # Шифруем нужны урлы вручную и сверяем с теми, что получились на выходе из прокси
    for link in [y for x, y in original_json.items() if x.startswith('crypted_url')]:
        assert crypt_url(binurlprefix, link, key, True) in crypted_links


@pytest.mark.parametrize('crypted_host', [
    None,
    'test.local',
    'test.com'])
@pytest.mark.parametrize('url', ('http://yastatic.net/partner-code/PCODE_CONFIG.js', 'http://yastat.net/partner-code/PCODE_CONFIG.js'))
@pytest.mark.usefixtures('stub_server')
def test_client_side_replaces_with_crypted_host_param(cryprox_worker_address, set_handler_with_config, get_config, crypted_host, url):
    """
    Проверяем, что параметр CRYPTED_HOST корректно подставляется в префикс шифрования при передаче на фронт
    :param crypted_host: значение параметра CRYPTED_HOST в конфиге партнера
    """

    test_config = get_config('test_local')
    new_test_config = test_config.to_dict()
    new_test_config['CRYPTED_HOST'] = crypted_host

    set_handler_with_config(test_config.name, new_test_config)

    seed = 'my2007'
    key = get_key(test_config.CRYPT_SECRET_KEY, seed)

    host = crypted_host if crypted_host is not None else 'test.local'

    binurlprefix = CryptUrlPrefix('http', host, seed, test_config.CRYPT_URL_PREFFIX)

    crypted_url = urlparse(crypt_url(binurlprefix, url, key, False, origin='test.local'))
    crypted_url = crypted_url._replace(netloc=cryprox_worker_address).geturl()

    proxied_data = requests.get(crypted_url, headers={'host': 'test.local',
                                                      SEED_HEADER_NAME: seed,
                                                      PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]}).text
    assert host in binurlprefix.crypt_prefix()
    assert binurlprefix.crypt_prefix() in proxied_data


@pytest.mark.parametrize('service_id, host, url, host_override, expected_host', [
    ('zen.yandex.ru', 'zen.yandex.ru', 'http://zen.yandex.ru/smth', None, 'zen.yandex.ru'),
    ('zen.yandex.ru', 'zen.yandex.ru', 'http://zen.yandex.ru/smth', 'hostoverride.zen.yandex.ru', 'hostoverride.zen.yandex.ru'),
    ('autoru', 'auto.ru', 'http://avatars.mds.yandex.net/get-auto/a', None, 'auto.ru'),
    ('autoru', 'auto.ru', 'http://avatars.mds.yandex.net/get-auto/a', 'override.com', 'auto.ru'),
])
def test_crypted_host_override(cryprox_worker_url, stub_server, get_key_and_binurlprefix_from_config, get_config, service_id, host, url, host_override, expected_host):
    """
    Проверяем, что crypted_host для партнера zen.yandex.ru может быть переопределен через заголовок x-aab-host (а для других -- нет)
    """

    config = get_config(service_id)
    seed = 'my2007'

    def handler(**_):
        body = {'url': url}
        headers = {'Content-Type': 'application/json'}
        if host_override is not None:
            headers['X-Aab-Host'] = host_override
        return {'text': json.dumps(body), 'code': 200, 'headers': headers}
    stub_server.set_handler(handler)

    response = requests.get(urljoin(cryprox_worker_url, '/data'),
                            headers={'host': host,
                                     SEED_HEADER_NAME: seed,
                                     PARTNER_TOKEN_HEADER_NAME: config.PARTNER_TOKENS[0]
                                     })
    obj = json.loads(response.text)
    crypted_url = urlparse(str(obj['url']))
    decrypted_url, url_seed, origin = decrypt_url(crypted_url._replace(scheme='', netloc='').geturl(), config.CRYPT_SECRET_KEY, str(config.CRYPT_URL_PREFFIX), False)
    assert url_seed == seed
    assert origin == expected_host
