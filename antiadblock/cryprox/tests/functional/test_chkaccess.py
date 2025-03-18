# -*- coding: utf8 -*-

import pytest
import requests
from urlparse import urljoin

from antiadblock.cryprox.cryprox.common.cryptobody import crypt_url
from antiadblock.cryprox.cryprox.config.system import PARTNER_TOKEN_HEADER_NAME, DETECT_LIB_HOST, DETECT_LIB_PATHS, NGINX_SERVICE_ID_HEADER

from antiadblock.cryprox.tests.lib.constants import cryprox_error_message
from antiadblock.cryprox.tests.lib.containers_context import TEST_LOCAL_HOST

valid_domains = (
    'an.yandex.ru',  # bk config
    'bs.yandex.ru',
    'yabs.yandex.ru',
    'yabs.yandex.by',
    'yabs.yandex.kz',
    'yabs.yandex.ua',
    'yabs.yandex.uz',
    'ads.adfox.ru',  # adfox config
    'banners.adfox.ru',
    'favicon.yandex.net/favicon',  # common config
    'an.yandex.ru/count',
    'an.yandex.ru/rtbcount',
    'an.yandex.ru/page',
    'an.yandex.ru/meta',
    'yandex.ru/an/count',
    'yandex.ru/an/rtbcount',
    'yandex.ua/an/rtbcount',
    'yandex.ru/ads/page',
    'yandex.ru/ads/meta',
    'yandex.com.am/ads/meta',
    'an.yandex.ru/blkset',
    'an.yandex.ru/system',
    'an.yandex.ru/resource',
    'an.yandex.ru/adfox',  # adfox config
    'yandex.ru/ads/adfox',  # adfox config
    'direct.yandex.ru',
    'yandexadexchange.net',
    'st.yandexadexchange.net',
    'avatars-fast.yandex.net',
    'avatars.mds.yandex.net/get-canvas',
    'avatars.mds.yandex.net/get-direct',
    'avatars.mds.yandex.net/get-rtb',
    'avatars.mds.yandex.net/get-yabs_performance',
    'storage.mds.yandex.net/get-bstor',
    'yastatic.net',
    'yastat.net',
    'test.local',  # test partner config
    'aab-pub.s3.yandex.net')


@pytest.mark.parametrize('domain,is_success_access',
                         [(domain, True) for domain in (valid_domains + ('{}/0/#fragment'.format(valid_domains[0]), ))] +
                         [(domain, False) for domain in ['test.' + domain for domain in valid_domains]])
@pytest.mark.usefixtures('stub_server')
def test_access(domain, is_success_access, get_key_and_binurlprefix_from_config, get_config):
    test_config = get_config('test_local')
    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)
    crypted_link = crypt_url(binurlprefix, 'http://{}/'.format(domain), key, True)
    response = requests.get(crypted_link,
                            headers={"host": TEST_LOCAL_HOST, PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]})

    assert response.headers.get(NGINX_SERVICE_ID_HEADER) == 'test_local'
    assert response.status_code in ((200, 404) if is_success_access else (403,))  # 404 for the case when access is allowed, but there is no resource
    if not is_success_access:
        assert cryprox_error_message == response.text


@pytest.mark.skip(reason="unskip after https://st.yandex-team.ru/ANTIADB-644")
@pytest.mark.usefixtures('stub_server')
def test_multi_token_access(get_key_and_binurlprefix_from_config, get_config):

    p_config = get_config('test_local')

    key, binurlprefix = get_key_and_binurlprefix_from_config(p_config)
    crypted_link = crypt_url(binurlprefix, 'http://{}/'.format(TEST_LOCAL_HOST), key, True)

    # Проверяем, что у партнера больше 1 токена, иначе тест не имеет смысла
    assert len(p_config.PARTNER_TOKENS) > 1

    for token in p_config.PARTNER_TOKENS:
        response = requests.get(crypted_link,
                                headers={"host": TEST_LOCAL_HOST, PARTNER_TOKEN_HEADER_NAME: token})
        assert response.status_code == 200


@pytest.mark.parametrize('pid, token_enabled, http_host, http_path, expected_code',
                         [("test_local", True, TEST_LOCAL_HOST, '/', 200),
                          ("test_local", False, TEST_LOCAL_HOST, '/', 403),
                          (None, True, TEST_LOCAL_HOST, '/', 200),
                          (None, False, TEST_LOCAL_HOST, '/', 403),
                          ('Fake', False, TEST_LOCAL_HOST, '/', 403),
                          ('Fake', True, TEST_LOCAL_HOST, '/', 200),

                          ("test_local", True, DETECT_LIB_HOST, '/', 200),
                          ("test_local", False, DETECT_LIB_HOST, '/', 403),
                          (None, True, DETECT_LIB_HOST, '/', 200),
                          (None, False, DETECT_LIB_HOST, '/', 403),
                          ('Fake', False, DETECT_LIB_HOST, '/', 403),
                          ('Fake', True, DETECT_LIB_HOST, '/', 200),

                          ("test_local", True, DETECT_LIB_HOST, DETECT_LIB_PATHS[0], 200),
                          ("test_local", False, DETECT_LIB_HOST, DETECT_LIB_PATHS[0], 200),
                          (None, True, DETECT_LIB_HOST, DETECT_LIB_PATHS[0], 200),
                          (None, False, DETECT_LIB_HOST, DETECT_LIB_PATHS[0], 403),
                          ('Fake', False, DETECT_LIB_HOST, DETECT_LIB_PATHS[0], 403),
                          ('Fake', True, DETECT_LIB_HOST, DETECT_LIB_PATHS[0], 403),

                          ("test_local", True, DETECT_LIB_HOST, DETECT_LIB_PATHS[1], 200),
                          ("test_local", False, DETECT_LIB_HOST, DETECT_LIB_PATHS[1], 200),
                          (None, True, DETECT_LIB_HOST, DETECT_LIB_PATHS[1], 200),
                          (None, False, DETECT_LIB_HOST, DETECT_LIB_PATHS[1], 403),
                          ('Fake', False, DETECT_LIB_HOST, DETECT_LIB_PATHS[1], 403),
                          ('Fake', True, DETECT_LIB_HOST, DETECT_LIB_PATHS[1], 403),

                          ("test_local", True, TEST_LOCAL_HOST, DETECT_LIB_PATHS[0], 200),
                          ("test_local", False, TEST_LOCAL_HOST, DETECT_LIB_PATHS[0], 403),
                          (None, True, TEST_LOCAL_HOST, DETECT_LIB_PATHS[0], 200),
                          (None, False, TEST_LOCAL_HOST, DETECT_LIB_PATHS[0], 403),
                          ('Fake', False, TEST_LOCAL_HOST, DETECT_LIB_PATHS[0], 403),
                          ('Fake', True, TEST_LOCAL_HOST, DETECT_LIB_PATHS[0], 200)])
def test_pid_and_token_combination_auth(cryprox_worker_url, pid, token_enabled, http_host, http_path, expected_code, get_config):

    test_url = urljoin(cryprox_worker_url, http_path)

    headers = dict(host=http_host)
    if pid:
        test_url += '?pid={}'.format(pid)
    if token_enabled:
        headers.update({PARTNER_TOKEN_HEADER_NAME: get_config('test_local').PARTNER_TOKENS[0]})

    response_code = requests.get(test_url, headers=headers).status_code

    assert response_code == expected_code
