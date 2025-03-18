# -*- coding: utf8 -*-
from urlparse import urljoin
from time import time

import pytest
import requests

from antiadblock.libs.decrypt_url.lib import get_key
from antiadblock.cryprox.cryprox.config import system as system_config
from antiadblock.cryprox.cryprox.common.cryptobody import get_script_key
from antiadblock.cryprox.cryprox.common.tools.crypt_cookie_marker import get_crypted_cookie_value, decrypt
from antiadblock.cryprox.cryprox.common.tools.misc import ConfigName


@pytest.mark.parametrize("headers", (
    {},
    {"X-Real-Ip": "192.168.0.13"},
    {"Accept-Language": "ru-ru,ru;q=0.8,en-us;q=0.6,en;q=0.4"},
    {"User-Agent": "Mozilla/5.0 (compatible; MSIE 9.0; Windows NT 6.1; WOW64; Trident/5.0; chromeframe/12.0.742.112)"},
    {"X-Real-Ip": "192.168.0.13", "User-Agent": "Mozilla/5.0 (compatible; MSIE 9.0; Windows NT 6.1; WOW64; Trident/5.0; chromeframe/12.0.742.112)"},
    {"X-Real-Ip": "192.168.0.13", "Accept-Language": "ru-ru,ru;q=0.8,en-us;q=0.6,en;q=0.4", "X-Forwarded-For": "127.0.0.1"},
    {"User-Agent": "Mozilla/5.0 (compatible; MSIE 9.0; Windows NT 6.1; WOW64; Trident/5.0; chromeframe/12.0.742.112)",
     "Accept-Language": "ru-ru,ru;q=0.8,en-us;q=0.6,en;q=0.4"},
    {"X-Real-Ip": "192.168.0.13", "User-Agent": "Mozilla/5.0 (compatible; MSIE 9.0; Windows NT 6.1; WOW64; Trident/5.0; chromeframe/12.0.742.112)",
     "Accept-Language": "ru-ru,ru;q=0.8,en-us;q=0.6,en;q=0.4"},
    {"X-Forwarded-For": "127.0.0.1", "User-Agent": "Mozilla/5.0 (compatible; MSIE 9.0; Windows NT 6.1; WOW64; Trident/5.0; chromeframe/12.0.742.112)",
     "Accept-Language": "ru-ru,ru;q=0.8,en-us;q=0.6,en;q=0.4"},
))
@pytest.mark.parametrize('config_name', ('test_local', 'test_local_2::active::None::None'))
def test_proxy_generate_cookie(cryprox_worker_url, get_config, headers, config_name):
    test_config = get_config(config_name)
    pid = ConfigName(config_name).service_id
    seed = "my2007"
    key = get_key(test_config.CRYPT_SECRET_KEY, seed)
    script_key = "".join(get_script_key(key, seed))
    headers["host"] = system_config.DETECT_LIB_HOST
    # получим значение шифрованной куки вручную (отличается только временем)
    # в случае отсутствия User-Agent requests проставляет дефолтное значение
    user_agent_def = requests.utils.default_headers().get("User-Agent", "")
    encrypted = get_crypted_cookie_value(test_config.COOKIE_CRYPT_KEY, headers.get("X-Real-Ip", headers.get("X-Forwarded-For", "")),
                                         headers.get("User-Agent", user_agent_def), headers.get("Accept-Language", ""), int(time()))
    url = urljoin(cryprox_worker_url, "/static/optional.js") + "?pid={}&script_key={}".format(pid, script_key)
    # получим значение шифрованной куки
    response = requests.get(url, headers=headers)
    crypted_cookie = response.text
    assert 200 == response.status_code
    assert response.headers.get(system_config.NGINX_SERVICE_ID_HEADER) == pid
    # расшифруем оба значения и сравним, разницу во времени генерации будем считать валидной, если она не более 1 минуты
    generation_time_expected, expected = decrypt(str(encrypted), test_config.COOKIE_CRYPT_KEY).split(None, 1)
    generation_time_real, real = decrypt(str(crypted_cookie), test_config.COOKIE_CRYPT_KEY).split(None, 1)
    assert expected == real
    assert abs(int(generation_time_expected) - int(generation_time_real)) <= 60


@pytest.mark.parametrize('config_name', ('test_local', 'test_local_2::active::None::None'))
def test_proxy_generate_cookie_update_cookie_crypt_key(cryprox_worker_url, get_config, set_handler_with_config, config_name):
    """Check generate cookie with new secret key (update config)"""
    test_config = get_config(config_name)
    pid = ConfigName(config_name).service_id
    seed = "my2007"
    new_secret_key = "new_partner_cookie_crypt_key"
    new_test_config = test_config.to_dict()
    new_test_config['COOKIE_CRYPT_KEY'] = new_secret_key
    set_handler_with_config(config_name, new_test_config)

    key = get_key(test_config.CRYPT_SECRET_KEY, seed)
    script_key = "".join(get_script_key(key, seed))
    ip = "192.168.0.13"
    user_agent = "Mozilla/5.0 (compatible; MSIE 9.0; Windows NT 6.1; WOW64; Trident/5.0; chromeframe/12.0.742.112)"
    accept_language = "ru-ru,ru;q=0.8,en-us;q=0.6,en;q=0.4"
    headers = {
        "host": system_config.DETECT_LIB_HOST,
        "User-Agent": user_agent,
        "Accept-Language": accept_language,
        "X-Real-Ip": ip,
    }
    # получим занчение шифрованной куки вручную (отличается только временем)
    encrypted = get_crypted_cookie_value(new_secret_key, ip, user_agent, accept_language, int(time()))
    url = urljoin(cryprox_worker_url, "/static/optional.js") + "?pid={}&script_key={}".format(pid, script_key)
    # получим значение шифрованной куки
    response = requests.get(url, headers=headers)
    crypted_cookie = response.text
    assert 200 == response.status_code
    assert response.headers.get(system_config.NGINX_SERVICE_ID_HEADER) == pid
    # расшифруем оба значения и сравним, отбросив время
    expected, real = [decrypt(str(cookie), new_secret_key).split(None, 1)[1] for cookie in
                      (encrypted, crypted_cookie)]
    assert expected == real


@pytest.mark.parametrize('config_name', ('test_local', 'test_local_2::active::None::None'))
def test_proxy_generate_cookie_bad_script_key(cryprox_worker_url, get_config, config_name):
    test_config = get_config(config_name)
    pid = ConfigName(config_name).service_id
    seed = "my2007"
    key = get_key(test_config.CRYPT_SECRET_KEY, seed)
    script_key = "".join(get_script_key(key, seed))
    ip = "192.168.0.13"
    user_agent = "Mozilla/5.0 (compatible; MSIE 9.0; Windows NT 6.1; WOW64; Trident/5.0; chromeframe/12.0.742.112)"
    accept_language = "ru-ru,ru;q=0.8,en-us;q=0.6,en;q=0.4"
    headers = {
        "host": system_config.DETECT_LIB_HOST,
        "User-Agent": user_agent,
        "Accept-Language": accept_language,
        "X-Real-Ip": ip,
    }

    url = urljoin(cryprox_worker_url, "/static/optional.js") + "?pid={}&script_key={}".format(pid, script_key[:-10])
    # получим значение шифрованной куки
    response = requests.get(url, headers=headers)
    assert response.headers.get(system_config.NGINX_SERVICE_ID_HEADER) == pid
    assert 403 == response.status_code
