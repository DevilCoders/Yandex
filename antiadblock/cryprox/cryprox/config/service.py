# -*- coding: utf8 -*-
import logging
import os
import json
import multiprocessing
import socket
from datetime import timedelta

from antiadblock.libs.tornado_redis.lib.dc import get_host_dc
from antiadblock.cryprox.cryprox.common.tools.misc import update_dict


HOST = ''  # listen all
HOSTNAME = socket.gethostname()
SERVICE_PORT = int(os.getenv('SERVICE_PORT', 8080))
WORKER_PORT = int(os.getenv('WORKER_PORT', 8081))
WORKERS_COUNT = int(os.getenv('WORKERS_COUNT', 0))  # less or equal to 0 means cpu_count
WORKER_TEST_CONTROL_PORTS = [int(port) for port in os.getenv('WORKER_TEST_CONTROL_PORTS', ",".join(str(p) for p in range(8180, 8180 + (WORKERS_COUNT or multiprocessing.cpu_count())))).split(",")]

DEFAULT_TORNADO_MAX_CLIENTS = 1000

# http://www.tornadoweb.org/en/stable/httpclient.html#tornado.simple_httpclient.SimpleAsyncHTTPClient.initialize
PROXY_CLIENT_CONFIG = {
    'max_clients': int(os.getenv('TORNADO_MAX_CLIENTS', DEFAULT_TORNADO_MAX_CLIENTS)),
    'hostname_mapping': {
        'static-mon.yandex.net': '::1',  # наш домен для раздачи расширенной либы с детектом и куки-матчингом
        'ads.adfox.ru': 'ads6.adfox.ru',  # Ходим в v6 хост Адфокса https://st.yandex-team.ru/ANTIADB-841
    }
}
PROXY_CLIENT_CONFIG = update_dict(json.loads(os.getenv('PROXY_CLIENT_CONFIG', '{}')), PROXY_CLIENT_CONFIG)


STUB_SERVER_HOST = os.getenv('STUB_SERVER_HOST', 'localhost')
STUB_SERVER_PORT = os.getenv('STUB_SERVER_PORT', 1060)

ENV_TYPE = os.getenv('ENV_TYPE', 'testing')
LOCAL_RUN = ENV_TYPE not in ('staging', 'production', 'load_testing')
END2END_TESTING = ENV_TYPE == 'load_testing' and os.getenv('END2END_TESTING', '0') == '1'

PROFILING = ENV_TYPE == 'load_testing' and os.getenv('PROFILING', '0') == '1'
CALLGRIND_FILENAME = 'run.callgrind'
CALLGRIND_STATS_UPDATE_PERIOD = int(os.getenv('CALLGRIND_STATS_UPDATE_PERIOD', 10 * 60 * 1000))  # in ms (10 min)
YAPPI_CLOCK_TYPE = os.getenv('YAPPI_CLOCK_TYPE', 'cpu')  # cpu or wall, https://github.com/sumerc/yappi/blob/master/doc/clock_types.md

LOG_PATH = os.getenv('CRYPROX_LOG_PATH', '/logs/cryprox')
LOG_FILE = os.path.join(LOG_PATH, 'cryprox.log')
RESPONSE_LOG_FILE = os.path.join(LOG_PATH, 'response.log')
END2END_TESTING_LOG_FILE = os.path.join(LOG_PATH, 'end2end_testing.log')
if not LOCAL_RUN and not os.path.exists(LOG_PATH):
    os.makedirs(LOG_PATH)

# If ENV_TYPE is not production update hostname_mapping with test domains, thus
# proxy would resolve these hostnames to STUB_SERVER
if ENV_TYPE in ['testing', 'load_testing']:
    hosts = ["test.local", "ads.adfox.ru", "ads6.adfox.ru", "an.yandex.ru", "yabs.yandex.ru", "yabs.yandex.by", "banners.adfox.ru", "favicon.yandex.net", "direct.yandex.ru",
             "yandexadexchange.net", "st.yandexadexchange.net", "avatars-fast.yandex.net", "avatars.mds.yandex.net", "storage.mds.yandex.net", "yastatic.net", "yastat.net",
             "aab-pub.s3.yandex.net", "dsp.rambler.ru", "ssp.rambler.ru", "img01.ssp.rambler.ru", "bs.yandex.ru",
             "mobile.yandexadexchange.net", "adsdk.yandex.ru", "yandex.ru", "zen.yandex.ru", "strm.yandex.net"]

    # TODO: use test.config instead
    hosts += ["auto.ru", "test.sport.s.rbc.ru", "aabturbo.gq"]

    test_mapping = {'hostname_mapping': {(domain, 80): (STUB_SERVER_HOST, STUB_SERVER_PORT) for domain in hosts}}
    PROXY_CLIENT_CONFIG = update_dict(PROXY_CLIENT_CONFIG, test_mapping)

if ENV_TYPE == 'testing':
    HOSTNAME = 'cryproxtest-sas-4.aab.yandex.net'

DC = get_host_dc(HOSTNAME)

DEFAULT_CONNECT_TIMEOUT = 5.0   # seconds
DEFAULT_REQUEST_TIMEOUT = 5.0   # seconds

PROXY_CLIENT_REQUEST_CONFIG = {'connect_timeout': float(os.getenv('CONNECT_TIMEOUT', DEFAULT_CONNECT_TIMEOUT)),
                               'request_timeout': float(os.getenv('REQUEST_TIMEOUT', DEFAULT_REQUEST_TIMEOUT)),
                               'allow_nonstandard_methods': True}

DEFAULT_LOGGING_LEVEL = 'WARNING'
LOGGING_LEVEL = logging.getLevelName(os.getenv('LOGGING_LEVEL', DEFAULT_LOGGING_LEVEL))


def boolify(value):
    return False if value.lower() == 'false' else True


CONFIGSAPI_CACHE_URL = 'http://[::1]:{}/v1/configs/'.format(SERVICE_PORT)
CONFIGSAPI_URL = os.getenv('CONFIGSAPI_URL', "https://preprod.aabadmin.yandex.ru/v2/configs_handler?status=active")
CRYPROX_TVM2_CLIENT_ID = int(os.getenv('CRYPROX_TVM2_CLIENT_ID', 2001023))  # default tvm client id is allowed only on dev env
CRYPROX_TVM2_SECRET = os.getenv('CRYPROX_TVM2_SECRET', 'rPjUoZxxsgx1Nr0sor_Z9A')  # default tvm client id is allowed only on dev env
ADMIN_TVM2_CLIENT_ID = int(os.getenv('ADMIN_TVM2_CLIENT_ID', 2000627))  # 2000627 is qloud dev stands default client id
CONFIG_UPDATE_PERIOD = int(os.getenv('CONFIG_UPDATE_PERIOD', 60 * 1000))  # How often do we poll cache and config service, in ms
SCRIPT_DETECT_UPDATE_PERIOD = int(os.getenv('SCRIPT_DETECT_UPDATE_PERIOD', 60 * 1000))

METRIC_GROUPING_TIME = 30  # s
METRICS_TRANSFER_DELAY = 1  # s
TO_CLEAR_SENSORS = True
if ENV_TYPE in ['testing']:
    METRIC_GROUPING_TIME = 1  # to avoid floating point numbers
    METRICS_TRANSFER_DELAY = 0.01  # s
    TO_CLEAR_SENSORS = False  # for correct functional tests

# retry settings for config fetching procedure. Default is [1, 3, 9] sec
FETCH_CONFIG_ATTEMPTS = int(os.getenv('FETCH_CONFIG_ATTEMPTS', 3))
FETCH_CONFIG_WAIT_INITIAL = int(os.getenv('FETCH_CONFIG_WAIT_INITIAL', 1))
FETCH_CONFIG_WAIT_MULTIPLIER = int(os.getenv('FETCH_CONFIG_WAIT_MULTIPLIER', 3))

# retry settings for resource fetching procedure. Default is [1, 3, 9] sec
FETCH_RESOURCE_ATTEMPTS = int(os.getenv('FETCH_RESOURCE_ATTEMPTS', 3))
FETCH_RESOURCE_WAIT_INITIAL = int(os.getenv('FETCH_RESOURCE_WAIT_INITIAL', 1))
FETCH_RESOURCE_WAIT_MULTIPLIER = int(os.getenv('FETCH_RESOURCE_WAIT_MULTIPLIER', 3))

PING_DELAY = timedelta(seconds=5)

COOKIE_ENCRYPTER_KEYS_PATH = os.getenv("COOKIEMATCHER_KEYS_PATH", "/etc/cookiematcher/cookiematcher_crypt_keys.txt")

REDIS_CONNECT_TIMEOUT = 0.5
REDIS_REQUEST_TIMEOUT_TD = timedelta(seconds=0.05)
REDIS_SENTINEL_UPDATE_PERIOD = 5
REDIS_HOSTS = [h.strip() for h in os.getenv('REDIS_HOSTS', 'sas-eg5ai7thhldbhokv.db.yandex.net').split(',')]
REDIS_SENTINEL_PORT = 26379
REDIS_PASSWORD = os.getenv('REDIS_PASSWORD', '').strip()
REDIS_SERVICE_NAME = 'antiadb_redis'

PERSISTENT_VOLUME_PATH = os.getenv('PERSISTENT_VOLUME_PATH', '/perm')
BYPASS_UIDS_FILE_PATH = os.getenv('BYPASS_UIDS_FILE_PATH', '/tmp_uids')
BYPASS_UIDS_UPDATE_PERIOD = 30 * 1000  # 30s in ms
INTERNAL_EXPERIMENT_CONFIG_UPDATE_PERIOD = 60 * 1000  # 60s in ms

COOKIELESS_PATH_PREFIX = '/test/' if ENV_TYPE in ['testing', 'staging'] else '/'
YASTATIC_DOMAINS = ("yastatic.net", "yastatic-net.ru")

STRM_TOKEN = os.getenv('STRM_TOKEN', '')
