# -*- coding: utf8 -*-
from urlparse import urljoin

import pytest
import requests
from hamcrest import assert_that, equal_to

from antiadblock.cryprox.cryprox.common.cryptobody import crypt_url
from antiadblock.cryprox.cryprox.config import system as system_config
from antiadblock.cryprox.tests.lib.util import update_internal_experiment_config_file


MOBILE_AGENT = 'Mozilla/5.0 (Linux; U; Android 2.2; ru-ru; Desire_A8181 Build/FRF91) AppleWebKit/533.1 (KHTML, like Gecko) Version/4.0 Mobile Safari/533.1'
DESKTOP_AGENT = 'Mozilla/5.0 (Macintosh; Intel Mac OS X 10.14; rv:69.0) Gecko/20100101 Firefox/69.0'


@pytest.mark.parametrize('is_exp', [False, True])
@pytest.mark.parametrize('x_real_ip,x_forwarded_for,is_internal', [
    ('', '', False),
    ('64.233.165.102', '64.233.165.113', False),
    ('5.45.192.1', '2a02:6b8::2:242', True),
    ('2a00:1450:4010:c08::8a', '2a02:6b8::2:242', True),
    ('', '5.45.192.1', True),
    ('', '64.233.165.113,5.45.192.1', False),
    ('', '5.45.192.1,64.233.165.113', True),
])
@pytest.mark.parametrize('crypt_content_handler', [False, True])
def test_internal_experiments(cryprox_worker_address, cryprox_service_url, stub_server, get_key_and_binurlprefix_from_config, get_config, update_handler_with_config,
                              is_exp, x_real_ip, x_forwarded_for, is_internal, crypt_content_handler):
    original_content = '<div class="some_class_1"><a href="http://test.local/experiment.js"></div>'
    expected_crypted_content = \
        '<div class="replaced_class_1"><a href="http://test.local/XWtN8S121/my2007kK/Kdf7P9akowI/poeTnSH/hKL07M/MADf7/m3cxu91/y76YNKPFNS/RQSX1/7Xpn0zBe/B6eevgDff7lhA/"></div>'

    def handler(**_):
        return {'text': original_content, 'code': 200, 'headers': {'Content-Type': 'text/html'}}

    if not crypt_content_handler:
        stub_server.set_handler(handler)

    exp_id = "exp_12345"
    if is_exp:
        update_internal_experiment_config_file({"test_local_2": exp_id})
    else:
        update_internal_experiment_config_file({})
    requests.get("{}/control/update_experiment_config".format(cryprox_service_url))

    active_config_name = "test_local_2::active::None::None"
    exp_config_name = "test_local_2::None::None::{}".format(exp_id)

    test_config = get_config(active_config_name)
    new_config = test_config.to_dict()
    new_config['REPLACE_BODY_RE'] = {r'\bsome_class_': 'replaced_class_'}
    new_config['CRYPT_URL_RE'] = [r'test\.local/experiment\.js']

    update_handler_with_config(exp_config_name, new_config)

    headers = {
        'x-real-ip': x_real_ip,
        'x-forwarded-for': x_forwarded_for,
        'host': 'test.local',
        system_config.PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0],
        system_config.SEED_HEADER_NAME: 'my2007',
    }
    if crypt_content_handler:
        proxied = requests.post(urljoin("http://" + cryprox_worker_address, '/crypt_content'), data=original_content, headers=headers)
    else:
        key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)
        crypted_link = crypt_url(binurlprefix, "http://test.local/1.html", key, False, origin='test.local')
        proxied = requests.get(crypted_link, headers=headers)

    assert proxied.status_code == 200
    if is_internal and is_exp:
        assert_that(proxied.text, equal_to(expected_crypted_content))
    else:
        assert_that(proxied.text, equal_to(original_content))


@pytest.mark.parametrize('x_real_ip,x_forwarded_for,is_internal', [
    ('', '64.233.165.113,5.45.192.1', False),
    ('', '5.45.192.1,64.233.165.113', True),
])
@pytest.mark.parametrize('crypt_content_handler', [False, True])
@pytest.mark.parametrize('config_device_type', ['desktop', 'mobile'])
@pytest.mark.parametrize('user_agent', [MOBILE_AGENT, DESKTOP_AGENT])
def test_internal_experiments_with_device_type(cryprox_worker_address, cryprox_service_url, stub_server, get_key_and_binurlprefix_from_config, get_config, update_handler_with_config,
                                               x_real_ip, x_forwarded_for, is_internal, crypt_content_handler, config_device_type, user_agent):
    original_content = '<div class="some_class_1"><a href="http://test.local/experiment.js"></div>'
    expected_crypted_content = \
        '<div class="replaced_class_1"><a href="http://test.local/XWtN8S121/my2007kK/Kdf7P9akowI/poeTnSH/hKL07M/MADf7/m3cxu91/y76YNKPFNS/RQSX1/7Xpn0zBe/B6eevgDff7lhA/"></div>'

    def handler(**_):
        return {'text': original_content, 'code': 200, 'headers': {'Content-Type': 'text/html'}}

    if not crypt_content_handler:
        stub_server.set_handler(handler)

    exp_id = "exp_12345"
    update_internal_experiment_config_file({"test_local_2": exp_id})
    requests.get("{}/control/update_experiment_config".format(cryprox_service_url))

    active_config_name = "test_local_2::active::None::None"
    exp_config_name = "test_local_2::None::{}::{}".format(config_device_type, exp_id)

    test_config = get_config(active_config_name)
    new_config = test_config.to_dict()
    new_config['REPLACE_BODY_RE'] = {r'\bsome_class_': 'replaced_class_'}
    new_config['CRYPT_URL_RE'] = [r'test\.local/experiment\.js']

    update_handler_with_config(exp_config_name, new_config)

    headers = {
        'x-real-ip': x_real_ip,
        'x-forwarded-for': x_forwarded_for,
        'user-agent': user_agent,
        'host': 'test.local',
        system_config.PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0],
        system_config.SEED_HEADER_NAME: 'my2007',
    }
    if crypt_content_handler:
        proxied = requests.post(urljoin("http://" + cryprox_worker_address, '/crypt_content'), data=original_content, headers=headers)
    else:
        key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)
        crypted_link = crypt_url(binurlprefix, "http://test.local/1.html", key, False, origin='test.local')
        proxied = requests.get(crypted_link, headers=headers)

    assert proxied.status_code == 200
    is_device_exp = (config_device_type == 'desktop' and user_agent == DESKTOP_AGENT) or (config_device_type == 'mobile' and user_agent == MOBILE_AGENT)
    if is_internal and is_device_exp:
        assert_that(proxied.text, equal_to(expected_crypted_content))
    else:
        assert_that(proxied.text, equal_to(original_content))
