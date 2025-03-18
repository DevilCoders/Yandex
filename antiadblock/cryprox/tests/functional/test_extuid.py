# -*- coding: utf8 -*-
import re2
import time

import pytest
import requests

from urlparse import urljoin, urlparse

from antiadblock.cryprox.cryprox.common.cryptobody import crypt_url
from antiadblock.cryprox.cryprox.config import system as system_config, bk as bk_config, adfox as adfox_config

# Универсальные тестовые кейсы. Дальше в тесте они будут отформатированы с помощью string.format()
from antiadblock.cryprox.tests.lib.containers_context import TEST_LOCAL_HOST

DECRYPTED_COOKIE_VALUE = "6731871211515776902"
ENCRYPTED_COOKIE_VALUE_BY_TEST_KEYS = "euo9uyWw7d4iZjI08fRvZutgqjIcQMU3c4uHIgjAx6RlAUm+Y5No9Nts3stu5KxLxQIlvvVt0tfHF/27g8lK/ePOV5A="

TEST_CASES = [
    # каждый кейс: ({куки для запроса};
    # ожидаемые куки запросе за рекламкой;
    # неожидаемые куки в запросе за рекламой;
    # квери-арги, которые как бы были доклеены на фронте - https://st.yandex-team.ru/ANTIADB-350;
    # ожидаемые запросы, сгенерированные от прокси к крутилке)
    (  # кейс1: кука с extuid есть, квери-арг с extuid есть
        {'extuid': 'test_extuid_uid'},
        {},
        [system_config.YAUID_COOKIE_NAME],
        '&{extuid_param}=client_id',
        ['{intermediate_path}uri_echo?{additional_params}{tag_param}=test_extuid_tag&{extuid_param}=test_extuid_uid'],
    ),
    (  # кейс2: куки с extuid нет, квери-арг с extuid есть
        {},
        {},
        [system_config.YAUID_COOKIE_NAME],
        '&{extuid_param}=client_id',
        ['{intermediate_path}uri_echo?{additional_params}{extuid_param}=client_id&{tag_param}=test_extuid_tag'],
    ),
    (  # кейс3: куки с extuid нет, квери-арга с extuid нет. Нет айдишника пользователя совсем
        {},
        {},
        [system_config.YAUID_COOKIE_NAME],
        '',
        ['{intermediate_path}uri_echo?{additional_params}{tag_param}=test_extuid_tag'],
    ),
    (  # кейс4: кука с extuid есть, квери-арга с extuid нет
        {'extuid': 'test_extuid_uid'},
        {},
        [system_config.YAUID_COOKIE_NAME],
        '',
        ['{intermediate_path}uri_echo?{additional_params}{tag_param}=test_extuid_tag&{extuid_param}=test_extuid_uid'],
    ),
    (  # кейс5: есть вторая ожидаемая кука с extuid, квери-арг с extuid есть
        {'second-ext-uid': 'another_extuid_uid'},
        {},
        [system_config.YAUID_COOKIE_NAME],
        '&{extuid_param}=client_id',
        ['{intermediate_path}uri_echo?{additional_params}{tag_param}=test_extuid_tag&{extuid_param}=another_extuid_uid'],
    ),
    (  # кейс6: есть две ожидаемые куки с extuid, квери-арг с extuid есть
        {'extuid': 'test_extuid_uid', 'second-ext-uid': 'another_extuid_uid'},
        {},
        [system_config.YAUID_COOKIE_NAME],
        '&{extuid_param}=client_id',
        ['{intermediate_path}uri_echo?{additional_params}{tag_param}=test_extuid_tag&{extuid_param}=test_extuid_uid',
         '{intermediate_path}uri_echo?{additional_params}{tag_param}=test_extuid_tag&{extuid_param}=another_extuid_uid'],
    ),
    (  # кейс7: кука с extuid есть, квери-арг с extuid есть, но один из квери-аргов - эникод-символ
        {'extuid': 'test_extuid_uid'},
        {},
        [system_config.YAUID_COOKIE_NAME],
        '&uni=✓&{extuid_param}=client_id',
        ['{intermediate_path}uri_echo?{additional_params}uni=%E2%9C%93&{tag_param}=test_extuid_tag&{extuid_param}=test_extuid_uid'],
    ),
    (  # case for request with crypted yandexuid: https://st.yandex-team.ru/ANTIADB-575
        {'crookie': ENCRYPTED_COOKIE_VALUE_BY_TEST_KEYS},
        {system_config.YAUID_COOKIE_NAME: DECRYPTED_COOKIE_VALUE},
        [],
        '',
        ['{intermediate_path}uri_echo?{additional_params}'],
    ),
    (  # case for request with crypted yandexuid: https://st.yandex-team.ru/ANTIADB-575
        {'crookie': 'BadCr1ptedC00k13'},
        {},
        [],
        '',
        ['{intermediate_path}uri_echo?{additional_params}{tag_param}=test_extuid_tag'],
    ),
    (  # case for request with crypted yandexuid: https://st.yandex-team.ru/ANTIADB-575
        {'crookie': 'BlankResponse'},
        {},
        [],
        '',
        ['{intermediate_path}uri_echo?{additional_params}{tag_param}=test_extuid_tag'],
    ),
    (  # case for request with crypted yandexuid: https://st.yandex-team.ru/ANTIADB-575
        {'crookie': ENCRYPTED_COOKIE_VALUE_BY_TEST_KEYS, 'extuid': 'test_extuid_uid'},
        {system_config.YAUID_COOKIE_NAME: DECRYPTED_COOKIE_VALUE},
        [],
        '',
        ['{intermediate_path}uri_echo?{additional_params}'],
    ),
    (  # case for request with crypted yandexuid: https://st.yandex-team.ru/ANTIADB-575
        {'crookie': 'BadCr1ptedC00k13', 'extuid': 'test_extuid_uid'},
        {},
        [],
        '',
        ['{intermediate_path}uri_echo?{additional_params}{tag_param}=test_extuid_tag&{extuid_param}=test_extuid_uid'],
    ),
    (  # case for request with crypted yandexuid: https://st.yandex-team.ru/ANTIADB-575
        {'crookie': 'BlankResponse', 'extuid': 'test_extuid_uid'},
        {},
        [],
        '',
        ['{intermediate_path}uri_echo?{additional_params}{tag_param}=test_extuid_tag&{extuid_param}=test_extuid_uid'],
    ),
    (  # case for request with crypted yandexuid: https://st.yandex-team.ru/ANTIADB-575
        {'crookie': ENCRYPTED_COOKIE_VALUE_BY_TEST_KEYS},
        {system_config.YAUID_COOKIE_NAME: DECRYPTED_COOKIE_VALUE},
        [],
        '&{extuid_param}=client_id',
        ['{intermediate_path}uri_echo?{additional_params}'],
    ),
    (  # case for request with crypted yandexuid: https://st.yandex-team.ru/ANTIADB-575
        {'crookie': 'BadCr1ptedC00k13'},
        {},
        [],
        '&{extuid_param}=client_id',
        ['{intermediate_path}uri_echo?{additional_params}{extuid_param}=client_id&{tag_param}=test_extuid_tag'],
    ),
    (  # case for request with crypted yandexuid: https://st.yandex-team.ru/ANTIADB-575
        {'crookie': 'BlankResponse'},
        {},
        [],
        '&{extuid_param}=client_id',
        ['{intermediate_path}uri_echo?{additional_params}{extuid_param}=client_id&{tag_param}=test_extuid_tag'],
    ),
]


def handler(**request):
    path = request.get('path', '/')
    if path in ['/adfox/123/getBulk/uri_echo', '/123/getCode/uri_echo', '/meta/uri_echo', '/count/uri_echo']:
        headers = {'x-aab-uri': request['path'] + '?' + request.get('query', '')}
        if system_config.YAUID_COOKIE_NAME in request['cookies']:
            headers.update({'cookie_' + system_config.YAUID_COOKIE_NAME: request['cookies'][system_config.YAUID_COOKIE_NAME]})
        return {'text': 'ok', 'code': 200, 'headers': headers}
    # Test stub for yandexuid decrypt api [https://st.yandex-team.ru/ANTIADB-575]
    if path.startswith('/decrypt/'):
        raise Exception("deprecated handler")
    return {}


@pytest.mark.parametrize('request_cookies, expected_req_cookies, not_expected_req_cookies, clientside_query_args, expected_uris', TEST_CASES)
def test_bk_adfox_extuid_cookie_scenarios(stub_server,
                                          request_cookies,
                                          expected_req_cookies,
                                          not_expected_req_cookies,
                                          clientside_query_args,
                                          expected_uris,
                                          get_config,
                                          get_key_and_binurlprefix_from_config):
    """
    Test various combinations of ext-uid cookie and query args with ext-uid generated by client side JS
    :param cookies: client cookies
    :param clientside_query_args: query args to be added by client side JS
    :expected_uris: expected uri path request to rtb generated by cryprox
    """
    def sort_querystring(url):
        parsed = urlparse(url)
        parsed = parsed._replace(query='&'.join(sorted(parsed.query.split('&'))))
        return parsed.geturl()

    stub_server.set_handler(handler)
    an_bk_count = {'url_base': 'http://an.yandex.ru/',
                   'intermediate_path': '/count/',
                   'additional_params': 'adb_enabled=1&redir-setuniq=1&test-tag=562949953421313&adb-bits=1&',
                   'tag_param': bk_config.BK_EXTUID_TAG_PARAM,
                   'extuid_param': bk_config.BK_EXTUID_PARAM}
    an_bk = {'url_base': 'http://an.yandex.ru/',
             'intermediate_path': '/meta/',
             'additional_params': 'adb_enabled=1&callback=json&disabled-ad-type=56&redir-setuniq=1&adb-bits=1&',
             'tag_param': bk_config.BK_EXTUID_TAG_PARAM,
             'extuid_param': bk_config.BK_EXTUID_PARAM}
    yabs_bk = an_bk.copy()
    yabs_bk['url_base'] = 'http://yabs.yandex.ru/'

    adfox = {'url_base': 'http://ads.adfox.ru/',
             'intermediate_path': '/123/getCode/',
             'additional_params': 'adb_enabled=1&disabled-ad-type=56&adb-bits=1&',
             'tag_param': adfox_config.ADFOX_EXTUID_TAG_PARAM,
             'extuid_param': adfox_config.ADFOX_EXTUID_PARAM}

    an_adfox = {'url_base': 'http://an.yandex.ru/',
                'intermediate_path': '/adfox/123/getBulk/',
                'additional_params': 'adb_enabled=1&disabled-ad-type=56&redir-setuniq=1&adb-bits=1&',
                'tag_param': adfox_config.ADFOX_EXTUID_TAG_PARAM,
                'extuid_param': adfox_config.ADFOX_EXTUID_PARAM}

    test_config = get_config('test_local')
    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)
    for source in [an_bk_count, an_bk, yabs_bk, adfox, an_adfox]:
        url = urljoin(source['url_base'], source['intermediate_path']) + 'uri_echo?' + clientside_query_args.format(extuid_param=source['extuid_param'])
        crypted_link = crypt_url(binurlprefix, url, key, True, origin='test.local')
        response = requests.get(crypted_link,
                                headers={'host': TEST_LOCAL_HOST, system_config.PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]},
                                cookies=request_cookies)

        formatted_expected_uris = []
        for uri in expected_uris:
            formatted_expected_uris.append(sort_querystring(uri.format(intermediate_path=source['intermediate_path'],
                                                                       additional_params=source['additional_params'],
                                                                       tag_param=source['tag_param'],
                                                                       extuid_param=source['extuid_param']).rstrip('&')))

        assert response.text == 'ok'
        assert sort_querystring(response.headers['x-aab-uri']) in formatted_expected_uris
        for cookie in not_expected_req_cookies:
            assert response.headers.get('cookie_' + cookie, None) is None
        for cookie, value in expected_req_cookies.items():
            assert response.headers.get('cookie_' + cookie, None) == value


@pytest.mark.skip(reason="no container logs any more")
@pytest.mark.parametrize('request_cookies, expected_log_entry', [
    (  # c кукой extuid
        {'extuid': 'test_extuid_uid'},
        '''INFO.*"cm_type": "with_extuid"''',  # entry should be not under DEBUG level
    ),
    (  # без куки
        {},
        '''INFO.*"cm_type": "without_uid"''',
    ),
    (  # с шифрованной кукой
        {'crookie': 'Cr1P73d'},
        '''INFO.*"cm_type": "with_crypted_yauid"''',
    ),
    (  # с yandexuid'ом
        {'yandexuid': 'someUid'},
        '''INFO.*"cm_type": "with_yauid"''',
    ),
])
def test_log_cookiematching_type(stub_server, request_cookies, expected_log_entry, get_config, get_key_and_binurlprefix_from_config, containers_context):
    """
    Test various combinations of ext-uid cookie and query args with ext-uid generated by client side JS
    :param cookies: client cookies
    :param clientside_query_args: query args to be added by client side JS
    :expected_uris: expected uri path request to rtb generated by cryprox
    """
    stub_server.set_handler(handler)

    test_config = get_config('test_local')
    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)
    for url in ['http://an.yandex.ru/count/', 'http://an.yandex.ru/meta/', 'http://ads.adfox.ru/123/getCode/']:
        crypted_link = crypt_url(binurlprefix, url, key, True, origin='test.local')
        request_time = int(time.time())
        requests.get(crypted_link,
                     headers={'host': TEST_LOCAL_HOST,
                              system_config.PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]},
                     cookies=request_cookies)

        (cryprox, _) = containers_context
        logs = cryprox.logs(since=request_time)
        assert re2.search(expected_log_entry, logs) is not None
