# -*- coding: utf8 -*-

from urlparse import urljoin
import json

import pytest
import requests

from antiadblock.cryprox.cryprox.config import system as system_config
from antiadblock.cryprox.tests.lib.containers_context import TEST_LOCAL_HOST


def echo_cookie_handler(**request):
    """
    Простейший хендлер, имитирующий партнера, который в теле ответа возвращает куки запроса в json-словаре
    """
    if request.get('path', '/') == '/echo_cookie':
        cookies = request.get('cookies', {})
        return {'text': json.dumps(cookies), 'code': 200}
    return {'text': 'Not expected path', 'code': 404}


@pytest.mark.parametrize('request_cookies, expected_cookies', [
    ({}, {}),  # Запрос совсем без кук, проверяем, что не падаем и не добавляем ничего лишнего от себя
    ({'yandexid': '123', 'test_cookie': '1', 'bltsr': '1'}, {'yandexid': '123'}),  # Кука test_cookie вырезается в конфиге партнера test, bltsr - для всех партнеров
    ({'yandexid': '123', 'uid': '1jdhd747dhd', 'context': 'same'}, {'yandexid': '123', 'uid': '1jdhd747dhd', 'context': 'same'}),  # ничего не вырезается
    ({'test_cookie': '1', 'bltsr': '1'}, {})])  # Кука test_cookie вырезается в конфиге партнера test, bltsr - для всех партнеров. Запрос без кук совсем в итоге
def test_proxy_cookies(stub_server, cryprox_worker_url, get_config, request_cookies, expected_cookies):
    """
    Проверка работы прокси с куками запроса - удаление, добавление новых, проксирование партнерских и тп
    :param request_cookies: изначальные куки запроса в прокси
    :param expected_cookies: ожидаемые куки, с которыми прокси сделает запрос дальше
    """
    stub_server.set_handler(echo_cookie_handler)
    response_text = requests.get(urljoin(cryprox_worker_url, '/echo_cookie'),
                                 headers={'host': TEST_LOCAL_HOST, system_config.PARTNER_TOKEN_HEADER_NAME: get_config('test_local').PARTNER_TOKENS[0]},
                                 cookies=request_cookies).text

    assert json.loads(response_text) == expected_cookies
