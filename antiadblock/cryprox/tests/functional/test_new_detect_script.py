# -*- coding: utf8 -*-
import pytest
import requests
from urlparse import urljoin

from antiadblock.cryprox.cryprox.common.cryptobody import crypt_url
from antiadblock.cryprox.cryprox.config.system import DETECT_LIB_HOST, PARTNER_TOKEN_HEADER_NAME, SEED_HEADER_NAME, CRYPTED_HOST_HEADER_NAME,\
    DEFAULT_DETECT_LIB_PATH, TEST_DETECT_LIB_PATH, WITHOUT_CM_DETECT_LIB_PATH, WITH_CM_DETECT_LIB_PATH, CM_DEFAULT_TYPE
from antiadblock.cryprox.cryprox.common.tools.misc import ConfigName
from antiadblock.cryprox.tests.lib.containers_context import TEST_LOCAL_HOST
from antiadblock.libs.tornado_redis.lib.dc import DataCenters


@pytest.mark.parametrize('dc,is_new_script', [
    ([], False),
    ([DataCenters.man], False),
    # для тестов переопределяем HOSTNAME = 'cryproxtest-sas-4.aab.yandex.net'
    ([DataCenters.sas], True),
    ([DataCenters.man, DataCenters.sas], True),
])
@pytest.mark.parametrize('config_name', ('test_local', 'test_local_2::active::None::None'))
def test_new_detect_script_per_dc(dc, is_new_script, config_name, stub_server, get_config, set_handler_with_config, cryprox_worker_url, get_key_and_binurlprefix_from_config):
    test_config = get_config(config_name)
    new_test_config = test_config.to_dict()
    new_test_config['NEW_DETECT_SCRIPT_URL'] = urljoin('http://' + DETECT_LIB_HOST, TEST_DETECT_LIB_PATH)
    new_test_config['NEW_DETECT_SCRIPT_DC'] = dc
    set_handler_with_config(config_name, new_test_config)
    pid = ConfigName(config_name).service_id
    url = urljoin(cryprox_worker_url, DEFAULT_DETECT_LIB_PATH) + '?pid={}'.format(pid)
    request_headers = {'host': DETECT_LIB_HOST, SEED_HEADER_NAME: 'my2007', CRYPTED_HOST_HEADER_NAME: 'test.local'}
    response = requests.get(url, headers=request_headers)
    assert response.status_code == 200
    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)
    if is_new_script:
        test_url = urljoin('http://' + DETECT_LIB_HOST, TEST_DETECT_LIB_PATH) + '?pid={}'.format(pid)
    else:
        test_url = urljoin('http://' + DETECT_LIB_HOST, DEFAULT_DETECT_LIB_PATH) + '?pid={}'.format(pid)
    proxied_with_pid_data = requests.get(crypt_url(binurlprefix, test_url, key, False, origin='test.local'),
                                         headers={"host": TEST_LOCAL_HOST,
                                                  PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]}).text
    assert response.text == proxied_with_pid_data


@pytest.mark.parametrize('enabled_cm', (True, False))
@pytest.mark.parametrize('enabled_change_detect', (True, False))
@pytest.mark.parametrize('config_name', ('test_local', 'test_local_2::active::None::None'))
def test_change_detect_script(enabled_cm, enabled_change_detect, config_name, stub_server, get_config, set_handler_with_config, cryprox_worker_url, get_key_and_binurlprefix_from_config):

    def handler(**request):
        path = request.get('path', '/')
        if path == DEFAULT_DETECT_LIB_PATH:
            content = "standart detect"
        elif path == WITHOUT_CM_DETECT_LIB_PATH:
            content = "internal detect"
        elif path == WITH_CM_DETECT_LIB_PATH:
            content = "external detect"
        else:
            return {'text': 'Unexpected request', 'code': 404}
        return {'text': content, 'code': 200, 'headers': {'Content-Type': 'text/javascript'}}

    stub_server.set_handler(handler)
    pid = ConfigName(config_name).service_id
    test_config = get_config(config_name)
    new_test_config = test_config.to_dict()

    if enabled_cm:
        new_test_config['CM_TYPE'] = CM_DEFAULT_TYPE
        expected = "external detect"
    else:
        new_test_config['CM_TYPE'] = []
        expected = "internal detect"
    new_test_config['AUTO_SELECT_DETECT'] = enabled_change_detect
    if not enabled_change_detect:
        expected = "standart detect"

    set_handler_with_config(config_name, new_test_config)

    url = urljoin(cryprox_worker_url, DEFAULT_DETECT_LIB_PATH) + '?pid={}'.format(pid)
    request_headers = {'host': DETECT_LIB_HOST, SEED_HEADER_NAME: 'my2007', CRYPTED_HOST_HEADER_NAME: 'test.local'}
    response = requests.get(url, headers=request_headers)
    assert response.status_code == 200
    assert response.text == expected
