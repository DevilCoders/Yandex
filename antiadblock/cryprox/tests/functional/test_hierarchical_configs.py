# -*- coding: utf8 -*-
import json
from urlparse import urljoin

import pytest
import requests
from hamcrest import assert_that, equal_to

from antiadblock.cryprox.cryprox.common.cryptobody import crypt_url
from antiadblock.cryprox.cryprox.config.system import PARTNER_TOKEN_HEADER_NAME, EXPERIMENT_ID_HEADER, DETECT_LIB_PATHS, DETECT_LIB_HOST
from antiadblock.cryprox.tests.lib.containers_context import TEST_LOCAL_HOST


def test_desktop_mobile_configs(stub_server, get_key_and_binurlprefix_from_config, get_config, update_handler_with_config):
    def handler(**_):
        return {'text': "success", 'code': 200, 'headers': {'Content-Type': 'text/html'}}

    stub_server.set_handler(handler)

    mobile_agent = 'Mozilla/5.0 (Linux; U; Android 2.2; ru-ru; Desire_A8181 Build/FRF91) AppleWebKit/533.1 (KHTML, like Gecko) Version/4.0 Mobile Safari/533.1'
    desktop_agent = 'Mozilla/5.0 (Macintosh; Intel Mac OS X 10.14; rv:69.0) Gecko/20100101 Firefox/69.0'

    config_name_template = 'test_local::active::{}::None'
    test_config = get_config('test_local')
    new_test_config = test_config.to_dict()

    key, binurlprefix = get_key_and_binurlprefix_from_config('test_local')

    for agent in (desktop_agent, mobile_agent):
        proxied = requests.get(crypt_url(binurlprefix, 'http://{}/test.bin'.format(TEST_LOCAL_HOST), key, False, origin='test.local'),
                               headers={"host": TEST_LOCAL_HOST, 'user-agent': agent,
                                        PARTNER_TOKEN_HEADER_NAME: new_test_config["PARTNER_TOKENS"][0]})

        assert proxied.status_code == 200

    config_name_desktop = config_name_template.format('desktop')
    update_handler_with_config(config_name_desktop, new_test_config)
    key, binurlprefix = get_key_and_binurlprefix_from_config(config_name_desktop)

    for (agent, status_code) in ((desktop_agent, 200), (mobile_agent, 403)):
        proxied = requests.get(crypt_url(binurlprefix, 'http://{}/test.bin'.format(TEST_LOCAL_HOST), key, False, origin='test.local'),
                               headers={"host": TEST_LOCAL_HOST, 'user-agent': agent,
                                        PARTNER_TOKEN_HEADER_NAME: new_test_config["PARTNER_TOKENS"][0]})

        assert proxied.status_code == status_code

    config_name_mobile = config_name_template.format('mobile')
    update_handler_with_config(config_name_mobile, new_test_config)
    key, binurlprefix = get_key_and_binurlprefix_from_config(config_name_mobile)

    for (agent, status_code) in ((desktop_agent, 403), (mobile_agent, 200)):
        proxied = requests.get(crypt_url(binurlprefix, 'http://{}/test.bin'.format(TEST_LOCAL_HOST), key, False, origin='test.local'),
                               headers={"host": TEST_LOCAL_HOST, 'user-agent': agent,
                                        PARTNER_TOKEN_HEADER_NAME: new_test_config["PARTNER_TOKENS"][0]})

        assert proxied.status_code == status_code


@pytest.mark.parametrize('exp_id', ('test_1', 'testing'))
def test_use_configs_with_exp_id(exp_id, stub_server, get_key_and_binurlprefix_from_config, get_config, update_handler_with_config):
    content = "<a href='https://www.somedomain.com/qqq'>"
    crypted_content = "<a href='http://test.local/XWtN7S903/my2007kK/Kdf_roahEiJ/pkeUXSJ/gKq05N/oZBqL/s38Qv8g/OgxYNUPFBP/VRmM2/bvupV3Xf/0TcefQBcM3l/'>"
    service_id = 'test_local_2'

    def handler(**_):
        return {'text': content, 'code': 200, 'headers': {'Content-Type': 'text/html'}}

    stub_server.set_handler(handler)

    test_config = get_config('{}::active::None::None'.format(service_id))
    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)
    new_test_config = test_config.to_dict()

    new_test_config['CRYPT_URL_RE'] = [r'(?:www\.)?somedomain\.com/.*?']
    config_name = '{}::test::None::None'.format(service_id) if exp_id == 'testing' else '{}::None::None::{}'.format(service_id, exp_id)
    update_handler_with_config(config_name, new_test_config)
    # выберется конфиг с exp_id и контент зашифруется
    headers = {"host": TEST_LOCAL_HOST, EXPERIMENT_ID_HEADER: exp_id, PARTNER_TOKEN_HEADER_NAME: new_test_config["PARTNER_TOKENS"][0]}
    proxied = requests.get(crypt_url(binurlprefix, 'http://{}/test.bin'.format(TEST_LOCAL_HOST), key, False, origin='test.local'), headers=headers)
    assert proxied.status_code == 200
    assert_that(proxied.text, equal_to(crypted_content))

    # конфига с exp_id=wrong_exp_id не существует, выберется стандартный и контент не зашифруется
    headers[EXPERIMENT_ID_HEADER] = 'wrong_exp_id'
    proxied = requests.get(crypt_url(binurlprefix, 'http://{}/test.bin'.format(TEST_LOCAL_HOST), key, False, origin='test.local'), headers=headers)
    assert proxied.status_code == 200
    assert_that(proxied.text, equal_to(content))


def test_config_for_pid_router(get_key_and_binurlprefix_from_config, get_config, cryprox_worker_address, update_handler_with_config):
    config_name_test = 'test_local_2::test::None::None'
    config_name_active = 'test_local_2::active::None::None'
    test_config = get_config(config_name_active)
    new_test_config = test_config.to_dict()
    new_test_config['CURRENT_COOKIE'] = 'somecookie'
    update_handler_with_config(config_name_test, new_test_config)

    response = requests.get(urljoin('http://' + cryprox_worker_address, DETECT_LIB_PATHS[0]) + '?pid=test_local_2', headers={'host': DETECT_LIB_HOST})
    assert response.status_code == 200
    text = response.text
    config = json.loads(text.split(' = ')[1].split(',"fn"')[0] + '}')
    assert config['cookieName'] == 'bltsr'
