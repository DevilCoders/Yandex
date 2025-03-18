import os
import logging
from collections import namedtuple

import pytest

import yatest.common
from .util import wait_for_availability
from antiadblock.libs.decrypt_url.lib import get_key
from antiadblock.cryprox.cryprox.common.cry import generate_seed
from antiadblock.cryprox.cryprox.common.cryptobody import CryptUrlPrefix

NGINX_PORT = 80
TEST_LOCAL_HOST = "test.local"
AUTO_RU_HOST = "auto.ru"

DEFAULT_COOKIELESS_HOST = '.naydex.net'

ThreadedServiceContext = namedtuple("ThreadedServiceContext", ["service_host_ip", "service_host_port", "worker_host_ip", "worker_host_port"])


@pytest.yield_fixture(scope="session")
def containers_context(stub_config_server_and_port, stub_server_and_port):
    persistent_volume_path = yatest.common.output_path("perm")
    if not os.path.exists(persistent_volume_path):
        os.makedirs(persistent_volume_path)

    with yatest.common.network.PortManager() as pm:
        _, stub_config_port = stub_config_server_and_port
        _, stub_port = stub_server_and_port
        service_port = pm.get_port()
        worker_port = pm.get_port()
        logging.warn(str((service_port, worker_port, stub_port, stub_config_port)))
        worker_count = 4
        test_control_ports = [str(pm.get_port()) for __ in range(worker_count)]
        logging.warning("Path is {}".format(yatest.common.binary_path("antiadblock/cryprox/cryprox_run/cryprox_run")))
        cryprox = None
        try:
            cryprox = yatest.common.execute(yatest.common.binary_path("antiadblock/cryprox/cryprox_run/cryprox_run"),
                                            check_exit_code=False,
                                            shell=False,
                                            timeout=None,
                                            cwd=None,
                                            env=dict(LOGGING_LEVEL="DEBUG",
                                                     ENV_TYPE="testing",
                                                     CONNECT_TIMEOUT="10.0",
                                                     REQUEST_TIMEOUT="10.0",
                                                     FETCH_CONFIG_WAIT_MULTIPLIER="1",
                                                     STUB_SERVER_HOST="localhost",  # stub_server_host,
                                                     STUB_SERVER_PORT=str(stub_port),  # stub_server_port,
                                                     CONFIGSAPI_URL="http://localhost:{}".format(stub_config_port),
                                                     SERVICE_PORT=str(service_port),
                                                     WORKER_PORT=str(worker_port),
                                                     COOKIEMATCHER_KEYS_PATH=yatest.common.source_path("antiadblock/encrypter/tests/test_keys.txt"),
                                                     BYPASS_UIDS_FILE_PATH=persistent_volume_path,
                                                     PERSISTENT_VOLUME_PATH=persistent_volume_path,
                                                     WORKER_TEST_CONTROL_PORTS=",".join(test_control_ports),
                                                     WORKERS_COUNT="4",
                                                     STRM_TOKEN="test_strm_token"),
                                            stdin=None,
                                            stdout=None,
                                            stderr=None,
                                            creationflags=0,
                                            wait=False,
                                            close_fds=False,
                                            collect_cores=True,
                                            check_sanitizer=True,
                                            on_timeout=None)
            wait_for_availability("http://{}:{}/ping".format("localhost",
                                                             service_port))

            yield ThreadedServiceContext(service_host_ip="localhost", service_host_port=service_port, worker_host_ip="localhost", worker_host_port=worker_port), None
        finally:
            if cryprox and cryprox.running:
                cryprox.kill()


@pytest.fixture(scope='session')
def cryprox_service_address(containers_context):
    cryprox_container, _ = containers_context
    return "{}:{}".format(cryprox_container.service_host_ip, cryprox_container.service_host_port)


@pytest.fixture(scope='session')
def cryprox_service_url(cryprox_service_address):
    return "http://{}".format(cryprox_service_address)


@pytest.fixture(scope='session')
def cryprox_worker_address(containers_context):
    cryprox_container, _ = containers_context
    return "{}:{}".format(cryprox_container.worker_host_ip, cryprox_container.worker_host_port)


@pytest.fixture(scope='session')
def cryprox_worker_url(cryprox_worker_address):
    return "http://{}".format(cryprox_worker_address)


@pytest.fixture
def get_key_and_binurlprefix_from_config(cryprox_worker_address, get_config):   # really bad name

    def impl(config, seed='my2007'):
        config = get_config(config) if isinstance(config, str) else config
        seed = seed or generate_seed()
        key = get_key(config.CRYPT_SECRET_KEY, seed)
        binurlprefix = CryptUrlPrefix('http', cryprox_worker_address, seed, config.CRYPT_URL_PREFFIX)
        return key, binurlprefix

    return impl
