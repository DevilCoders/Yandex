# -*- coding: utf8 -*-

import logging
import json

import pytest
import requests
from .initial_config import INITIAL_CONFIG

from antiadblock.cryprox.cryprox.common.config_utils import objview
from antiadblock.cryprox.cryprox.config.config import Config
from .stub_server import StubServer
from .util import wait_for_availability
from yatest.common import network


def initial_config():
    return INITIAL_CONFIG


@pytest.yield_fixture(scope='session', autouse=True)
def stub_config_server_and_port():

    with network.PortManager() as pm:
        config_stub_port = pm.get_port()

        def default_config_handler(**_):
            return {'text': initial_config(), 'code': 200}

        server = StubServer(port=config_stub_port)
        server.set_handler(default_config_handler)

        logging.warning("Starting stub config server")
        server.start()

        wait_for_availability("http://{}:{}/".format("localhost",
                                                     config_stub_port))

        yield server, config_stub_port

        server.stop()


@pytest.fixture(scope='session')
def stub_config_server(stub_config_server_and_port):
    stub_config_server, port = stub_config_server_and_port
    return stub_config_server


@pytest.yield_fixture
def restore_configs(stub_config_server, cryprox_service_url):

    yield

    def called(**_):
        return {'text': initial_config(), 'code': 200}

    stub_config_server.set_handler(called)

    # restore_configs is using in reload_configs. We can't use it here
    requests.get("{}/v1/control/reload_configs_cache".format(cryprox_service_url))
    requests.get("{}/v1/control/reload_configs".format(cryprox_service_url))


@pytest.yield_fixture
def set_handler_with_config(stub_config_server, reload_configs):

    def impl(name, config, version=None):

        def called(**_):
            return {'text': json.dumps({name: {'config': config, 'statuses': ['active'], 'version': version}}),
                    'code': 200}
        stub_config_server.set_handler(called)
        reload_configs()
    return impl


@pytest.yield_fixture
def update_handler_with_config(stub_config_server, reload_configs):
    # get initial_config and add a new config or update an existing one
    def impl(name, config, version=None):

        def called(**_):
            configs = json.loads(initial_config())
            configs[name] = {'config': config, 'statuses': ['active'], 'version': version}
            return {'text': json.dumps(configs),
                    'code': 200}
        stub_config_server.set_handler(called)
        reload_configs()
    return impl


@pytest.fixture
def reload_configs(restore_configs, cryprox_service_url):  # noqa
    def impl():
        response = requests.get("{}/v1/control/reload_configs_cache".format(cryprox_service_url))
        assert response.status_code == 200
        response = requests.get("{}/v1/control/reload_configs".format(cryprox_service_url))
        assert response.status_code == 200
    return impl


@pytest.mark.usefixtures("stub_config_server")
@pytest.fixture
def get_config(cryprox_service_url):

    def impl(name):
        response = requests.get('{}/v1/configs/'.format(cryprox_service_url))
        assert response.status_code == 200
        config = json.loads(response.content).get(name)
        if 'active' not in config.get('statuses'):
            config = None
        assert config is not None
        assert config.get('config') is not None
        return Config(name=name, config=objview(config['config']))
    return impl
