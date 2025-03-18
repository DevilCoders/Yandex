from json import loads, dumps

import pytest
import requests
from werkzeug.wrappers import Response

from antiadblock.cryprox.cryprox.config import system as system_config
from antiadblock.cryprox.cryprox.common.config_utils import FETCH_CONFIG_ATTEMPTS
from antiadblock.cryprox.cryprox.common.cryptobody import crypt_url
from antiadblock.cryprox.cryprox.config.system import SEED_HEADER_NAME, PARTNER_TOKEN_HEADER_NAME, DEBUG_TOKEN_HEADER_NAME, DEBUG_API_TOKEN, DEBUG_RESPONSE_HEADER_NAME
from antiadblock.cryprox.tests.lib.containers_context import TEST_LOCAL_HOST, AUTO_RU_HOST


@pytest.mark.usefixtures("restore_configs")
def test_config_cache_reload(stub_config_server, cryprox_service_url, get_config):
    expected_config = loads(dumps({"morda": {'config': get_config('test_local').to_dict(), 'statuses': ['active'],
                                             'version': 'test_config_cache_reload'}}))

    def called(**_):
        return {'text': dumps(expected_config), 'code': 200}

    stub_config_server.set_handler(called)
    requests.get("{}/v1/control/reload_configs_cache".format(cryprox_service_url))
    content = requests.get("{}/v1/configs/".format(cryprox_service_url)).content
    assert loads(content) == expected_config


@pytest.mark.usefixtures("restore_configs")
@pytest.mark.parametrize('new_config', ({1: '83'}, {}))
def test_config_cache_reload_fails(stub_config_server, new_config, cryprox_service_url):
    expected_config = loads(requests.get("{}/v1/configs/".format(cryprox_service_url)).content)

    def called(**_):
        return {'text': dumps(new_config), 'code': 500}

    stub_config_server.set_handler(called)

    requests.get("{}/v1/control/reload_configs_cache".format(cryprox_service_url))
    content = requests.get("{}/v1/configs/".format(cryprox_service_url)).content

    config_in_service = loads(content)
    assert config_in_service == expected_config


@pytest.mark.usefixtures("restore_configs")
def test_config_children_reload(stub_config_server, reload_configs, cryprox_worker_url, get_config):
    test_config = get_config('test_local')
    new_test_config = test_config.to_dict()
    new_test_config['PROXY_URL_RE'] = []
    new_test_config['CRYPT_URL_RE'] = []

    def called(**_):
        return {'text': dumps({test_config.name: {'config': new_test_config, 'statuses': ['active'],
                                                  'version': "test_config_children_reload"}}), 'code': 200}
    stub_config_server.set_handler(called)

    reload_configs()

    proxied_data = requests.get(cryprox_worker_url,
                                headers={"host": TEST_LOCAL_HOST,
                                         system_config.PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]})
    assert proxied_data.status_code == 403


@pytest.mark.usefixtures("restore_configs")
@pytest.mark.usefixtures("stub_server")
def test_new_child(stub_config_server, reload_configs, cryprox_worker_url, get_config, restore_stub):
    # Kostyl but don't know what else to do
    restore_stub()
    test_config = get_config('test_local')
    new_test_config = [{'config': test_config.to_dict(), 'statuses': ['active'], 'version': 0}]

    def called(**_):
        return {'text': dumps({}), 'code': 200}
    stub_config_server.set_handler(called)
    reload_configs()

    def called(**_):
        return {'text': dumps({'test2': new_test_config}), 'code': 200}
    stub_config_server.set_handler(called)
    reload_configs()

    proxied_data = requests.get(cryprox_worker_url,
                                headers={"host": TEST_LOCAL_HOST,
                                         system_config.PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]})

    assert proxied_data.status_code == 200


@pytest.mark.usefixtures("restore_configs")
def test_deleted_child(stub_config_server, cryprox_service_url, reload_configs, get_config):
    test_config = get_config('autoru')
    new_test_config = [{'config': test_config.to_dict(), 'statuses': ['active'], 'version': 0}]

    def called(**_):
        return {'text': dumps({'autoru': new_test_config}), 'code': 200}
    stub_config_server.set_handler(called)
    reload_configs()

    def called(**_):
        return {'text': dumps({'test2': new_test_config}), 'code': 200}
    stub_config_server.set_handler(called)

    proxied_data = requests.get(cryprox_service_url,
                                headers={"host": AUTO_RU_HOST,
                                         system_config.PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]})

    assert proxied_data.status_code == 403


@pytest.mark.usefixtures("restore_configs")
def test_retry_configs(reload_configs, get_config, stub_config_server, get_key_and_binurlprefix_from_config):
    """
    Checks that cryproxy could stay alive after (FETCH_CONFIG_ATTEMPTS - 1) failed fetch configs attempts
    :return:
    """
    config = get_config('test_local')

    class Handler(object):
        def __init__(self):
            self.response_generator = self.response()

        def response(self):
            for _ in xrange(FETCH_CONFIG_ATTEMPTS):
                yield Response(None, status=502)
            while True:
                yield Response(dumps(dict(test=dict(config=config.to_dict(), version="test", statuses=["active", "test"]))))

        def __call__(self, *args, **kwargs):
            return next(self.response_generator)

    stub_config_server.set_handler(Handler())
    reload_configs()

    key, binurlprefix = get_key_and_binurlprefix_from_config(config)
    url = crypt_url(binurlprefix, 'http://{}/'.format(TEST_LOCAL_HOST), key, True)
    response = requests.get(url, headers={'host': TEST_LOCAL_HOST,
                                          DEBUG_TOKEN_HEADER_NAME: DEBUG_API_TOKEN,
                                          SEED_HEADER_NAME: 'my2007',
                                          PARTNER_TOKEN_HEADER_NAME: config.PARTNER_TOKENS[0]})
    assert response.status_code == 200
    assert response.headers[DEBUG_RESPONSE_HEADER_NAME] == 'http://{}/'.format(TEST_LOCAL_HOST)
