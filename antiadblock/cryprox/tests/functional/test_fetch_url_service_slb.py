# -*- coding: utf8 -*-
import pytest
import requests
from urlparse import urlparse

from antiadblock.libs.decrypt_url.lib import get_key
from antiadblock.cryprox.cryprox.common.cryptobody import crypt_url, CryptUrlPrefix
from antiadblock.cryprox.cryprox.config.system import SEED_HEADER_NAME, PARTNER_TOKEN_HEADER_NAME
from antiadblock.cryprox.tests.lib.containers_context import DEFAULT_COOKIELESS_HOST


def handler(**request):
    path = request.get('path', '/')
    req_headers = {key.replace('HTTP_', '').replace('_', '-').lower(): val for key, val in request['headers'].items()}

    if path == '/images/dingdong':
        return {'text': '<html><div><a href="http://yastatic.net/PCODE_CONFIG.js">test</a></div></html>',
                'code': 200,
                'headers': {'Content-Type': 'application/javascript',
                            'x-test-host': req_headers.get('host'),
                            'x-test-path': path}}
    else:
        return {'text': 'What are u looking for?', 'code': 404}


@pytest.mark.parametrize('internal, cookieless_host_enabled, service_slb_url_re, xff_last_host, expected_code', [
    (True, False, ['thisissomefakedomain\.(?:ru|ua|by|net)/images/.*?'], "localhost", 200),
    (False, False, ['thisissomefakedomain\.(?:ru|ua|by|net)/images/.*?'], "localhost", 599),
    (True, False, [], "localhost", 599),
    (True, False, ['thisissomefakedomain\.(?:ru|ua|by|net)/images/.*?'], 'whatthehellfakedyanotdomain.com', 599),
    (True, True, ['thisissomefakedomain\.(?:ru|ua|by|net)/images/.*?'], 'localhost', 599),
])
def test_url_replace_with_regexp(cryprox_worker_address, set_handler_with_config, get_config, internal,
                                 cookieless_host_enabled, service_slb_url_re, xff_last_host, expected_code, stub_server_and_port):
    """
    Тесту нужен комментарий, он не очевидный.
    Суть в чем: домен thisissomefakedomain.ru не существует, не резолвится и в тест-стабе также не поддерживется.
    Соответственно, если пытаться фетчить урл с этим доменом, то получим 599. Чтобы вообще попытаться его сфетчить,
    мы добавляем его в PROXY_URL_RE партнера, чтобы прошел chkaccess.
    Далее, если срабатывает функциональность service_slb как ожидается, то запрос пойдет на тестовую стабу,
    но с хедером thisissomefakedomain.ru, и получит 200-ку из стабы этого теста, а не 599. Если же метод не отработает,
    то мы получим ожидаемую 599.
    Для cookieless_host (naydex.net) эта логика должна быть отключена и при запросе в него всегда 599.

    :param internal: флаг INTERNAL из конфига партнера
    :param cookieless_host_enabled: шифрование под найдекс включено или нет
    :param service_slb_url_re: значение SERVICE_SLB_URL_RE в конфиге партнера
    :param xff_last_host: последний хост в заголовке x-forwarded-for
    :param expected_code: ожидаемый код ответа
    """
    stub_server, service_slb_port = stub_server_and_port
    stub_server.set_handler(handler)

    test_config = get_config('test_local')
    new_test_config = test_config.to_dict()
    new_test_config['SERVICE_SLB_URL_RE'] = service_slb_url_re
    new_test_config['PROXY_URL_RE'].append(r'thisissomefakedomain\.(?:ru|ua|by|net)/.*')
    new_test_config['INTERNAL'] = internal
    new_test_config['ENCRYPT_TO_THE_TWO_DOMAINS'] = cookieless_host_enabled

    set_handler_with_config(test_config.name, new_test_config)

    seed = 'my2007'
    key = get_key(test_config.CRYPT_SECRET_KEY, seed)

    binurlprefix = CryptUrlPrefix('http', cryprox_worker_address, seed, test_config.CRYPT_URL_PREFFIX)

    uncrypted_url = 'http://thisissomefakedomain.ru/images/dingdong'
    crypted_url = crypt_url(binurlprefix, uncrypted_url, key, False, origin='test.local')

    proxied_crypted_data = requests.get(crypted_url,
                                        headers={'host': "test-local{}".format(DEFAULT_COOKIELESS_HOST) if cookieless_host_enabled else 'test.local',
                                                 SEED_HEADER_NAME: seed,
                                                 PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0],
                                                 'X-Forwarded-For': ','.join(['8.8.8.8', xff_last_host]),
                                                 'X-Yandex-Service-L7-Port': str(service_slb_port)})

    proxied_uncrypted_data = requests.get(urlparse(uncrypted_url)._replace(netloc=cryprox_worker_address).geturl(),
                                          headers={'host': 'thisissomefakedomain.ru',
                                                   SEED_HEADER_NAME: seed,
                                                   PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0],
                                                   'X-Forwarded-For': ','.join(['8.8.8.8', xff_last_host]),
                                                   'X-Yandex-Service-L7-Port': str(service_slb_port)})

    for proxied_data, host, code in [(proxied_crypted_data, 'test.local', expected_code),
                                     (proxied_uncrypted_data, 'thisissomefakedomain.ru', expected_code if not cookieless_host_enabled else 200)]:
        assert proxied_data.status_code == code
        if code == 200:
            # Проверяем, что не испортили заголовок Host, нам его возвращает тестовая стаба в заголовке
            assert proxied_data.headers['x-test-host'] == 'thisissomefakedomain.ru'
            # Проверяем, что не испортили uri path, также возвращается тестовой стабой в заголовке
            assert proxied_data.headers['x-test-path'] == '/images/dingdong'
            # Проверяем, что мы пошифровали контент с правильным доменом, а не с тем, что мы подставили для фетча
            assert proxied_data.text.find('http://{}'.format(host)) > 0


def test_service_slb_with_not_existed_port(cryprox_worker_address, set_handler_with_config, get_config, stub_server_and_port):
    """
    Тоже самое, что и в предыдущем тесте, но с несуществующим портом

    :param internal: флаг INTERNAL из конфига партнера
    :param service_slb_url_re: значение SERVICE_SLB_URL_RE в конфиге партнера
    :param xff_last_host: последний хост в заголовке x-forwarded-for
    :param service_slb_port: значение заголовка X-Yandex-Service-L7-Port - порт сервисного L7 балансера
    :param expected_code: ожидаемый код ответа
    """
    stub_server, stub_port = stub_server_and_port
    stub_server.set_handler(handler)

    # Точно несуществующий порт
    service_slb_port = stub_port - 1
    xff_last_host = "localhost"
    test_config = get_config('test_local')
    new_test_config = test_config.to_dict()
    new_test_config['SERVICE_SLB_URL_RE'] = [r'thisissomefakedomain\.(?:ru|ua|by|net)/images/.*']
    new_test_config['PROXY_URL_RE'].append(r'thisissomefakedomain\.(?:ru|ua|by|net)/.*?')
    new_test_config['INTERNAL'] = True

    set_handler_with_config(test_config.name, new_test_config)

    seed = 'my2007'
    key = get_key(test_config.CRYPT_SECRET_KEY, seed)

    binurlprefix = CryptUrlPrefix('http', cryprox_worker_address, seed, test_config.CRYPT_URL_PREFFIX)

    uncrypted_url = 'http://thisissomefakedomain.ru/images/dingdong'
    crypted_url = crypt_url(binurlprefix, uncrypted_url, key, False)

    proxied_crypted_data = requests.get(crypted_url,
                                        headers={'host': 'test.local',
                                                 SEED_HEADER_NAME: seed,
                                                 PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0],
                                                 'X-Forwarded-For': ','.join(['8.8.8.8', xff_last_host]),
                                                 'X-Yandex-Service-L7-Port': str(service_slb_port)})
    assert proxied_crypted_data.status_code == 599

    proxied_uncrypted_data = requests.get(urlparse(uncrypted_url)._replace(netloc=cryprox_worker_address).geturl(),
                                          headers={'host': 'thisissomefakedomain.ru',
                                                   SEED_HEADER_NAME: seed,
                                                   PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0],
                                                   'X-Forwarded-For': ','.join(['8.8.8.8', xff_last_host]),
                                                   'X-Yandex-Service-L7-Port': str(service_slb_port)})

    assert proxied_uncrypted_data.status_code == 599
