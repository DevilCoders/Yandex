import os
import logging
from json import dumps
from datetime import datetime, timedelta
from base64 import urlsafe_b64decode
from time import time
from monotonic import monotonic
from urlparse import parse_qs

import psutil as ps
import sys
import cPickle as pickle

import tornado.web
import tornado.gen as gen

from tornado.escape import json_encode
from tornado.gen import coroutine, with_timeout, TimeoutError
from tornado.httpclient import HTTPRequest
from tornado.simple_httpclient import SimpleAsyncHTTPClient
from cachetools import LRUCache

from antiadblock.cryprox.cryprox.common.config_utils import read_configs_from_api, read_configs_from_file_cache
from antiadblock.cryprox.cryprox.common.resource_utils import read_resource_from_url, url_to_filename
from antiadblock.cryprox.cryprox.common.juggler_utils import send_juggler_event
from antiadblock.cryprox.cryprox.config import system as system_config
from antiadblock.cryprox.cryprox.config.service import CONFIGSAPI_URL, WORKER_TEST_CONTROL_PORTS, PING_DELAY, \
    STUB_SERVER_PORT, STUB_SERVER_HOST, ENV_TYPE, REDIS_REQUEST_TIMEOUT_TD, PERSISTENT_VOLUME_PATH
from antiadblock.cryprox.cryprox.service.metrics import SOLOMON_METRICS_AGG, SYSTEM_METRICS, AppMetrics
from antiadblock.cryprox.cryprox.service.action import LOCAL_CACHE_GET, LOCAL_CACHE_PUT
from antiadblock.cryprox.cryprox.common.tools.porto_metrics import PORTO_METRICS

CACHE = LRUCache(maxsize=2 ** 30, getsizeof=lambda val: sys.getsizeof(val.data))


class ServiceApplication(tornado.web.Application):
    def __init__(self, handlers=None, tvm_client=None, default_host=None, transforms=None, **settings):
        self.configs_cache = None
        self.configs_cache_update_time = datetime.now()
        super(ServiceApplication, self).__init__(handlers, default_host, transforms, **settings)
        self.tvm_client = tvm_client
        self.start_time = datetime.now()
        if ENV_TYPE == 'testing':
            tornado.httpclient.AsyncHTTPClient.configure(None, resolver=None,
                                                         hostname_mapping={(system_config.DETECT_LIB_HOST, 80): (STUB_SERVER_HOST, STUB_SERVER_PORT)})

    @coroutine
    def update_configs_cache(self, configs=None):
        if datetime.now() - self.configs_cache_update_time > timedelta(minutes=20):
            send_juggler_event("Cryprox is using old configs", service="cryprox_config_cache_stale", status='CRIT')
        else:
            send_juggler_event("Cryprox is using old configs", service="cryprox_config_cache_stale", status='OK')

        if not configs:
            try:
                new_config = yield read_configs_from_api(CONFIGSAPI_URL, self.tvm_client)
            except Exception:
                if self.configs_cache is not None:
                    return  # We can't get fresh configs and got some in memory - so nothing else we can do

                try:
                    logging.info("Reading configs from file cache")
                    new_config = read_configs_from_file_cache()
                except Exception:
                    logging.exception("Can not read configs from file cache")
                    raise Exception("Can not read configs (nor from api, nor from cache) - failed to start")

            if new_config is not None:
                old_versions = {config['version'] for config in self.configs_cache.values()} if self.configs_cache is not None else set()
                new_versions = {config['version'] for config in new_config.values()}
                self.configs_cache = new_config
                self.configs_cache_update_time = datetime.now()  # TODO: Right now update time will be renewed even if we've read configs from file cache
                if old_versions != new_versions:
                    logging.info("Configs configuration was changed", enabled_configs=list(new_versions - old_versions), disabled_configs=list(old_versions - new_versions), action='config_updated')
        else:
            self.configs_cache = configs
            self.configs_cache_update_time = datetime.now()

    @coroutine
    def update_script_detect_cache(self):
        headers = {"host": system_config.DETECT_LIB_HOST}
        for lib_path in system_config.DETECT_LIB_PATHS:
            url = "http://{}{}".format(system_config.DETECT_LIB_HOST, lib_path)
            try:
                new_detect_cache = yield read_resource_from_url(url, headers)
                with open(os.path.join(PERSISTENT_VOLUME_PATH, url_to_filename(lib_path)), "wb") as f:
                    f.write(new_detect_cache)
            except Exception:
                logging.error("Can not read script detect from {}".format(url))

    @coroutine
    def start_pipeline_loop(self, pipeline):
        while True:
            pipeline.run_listener_iteration()
            yield gen.sleep(0.01)

    @coroutine
    def measure_cpu(self):
        while True:
            cpu_percent = PORTO_METRICS.cpu_percent()
            cpu_percent = cpu_percent if cpu_percent >= 0 else ps.cpu_percent()
            SYSTEM_METRICS.append_cpu_load(ts=int(time()), cpu_load_percent=cpu_percent)
            yield gen.sleep(0.1)

    @staticmethod
    def process_system_metrics(ts):
        SYSTEM_METRICS.append_request(ts)

    @staticmethod
    def aggr_metrics_from_list(container):
        for child_metrics in container:
            SOLOMON_METRICS_AGG.aggregate(child_metrics)
        del container[:]


class PingHandler(tornado.web.RequestHandler):

    @tornado.web.asynchronous
    def get(self):
        if datetime.now() - self.application.start_time > PING_DELAY:
            self.write('Ok.\n')
        else:
            self.set_status(500)
            self.write(u"Workers is not started")
        self.finish()


class ServiceHandler(tornado.web.RequestHandler):

    @tornado.web.asynchronous
    def get(self):
        request_id = self.request.headers.get(system_config.REQUEST_ID_HEADER_NAME, 'None')
        logging.debug(u'Unknown client address. RequestId: {}'.format(request_id))
        self.set_status(403)
        self.write(u'Unknown client address\n')
        self.finish()


class ConfigsHandler(tornado.web.RequestHandler):
    @tornado.web.asynchronous
    def get(self):
        self.write(dumps(self.application.configs_cache))
        self.finish()


class ControlHandler(tornado.web.RequestHandler):

    # noinspection PyMethodOverriding,PyAttributeOutsideInit
    def initialize(self, children):
        self.children = children

    @coroutine
    def get(self):
        logging.debug("Service process: Got request to update children configs", pids=self.children)
        client = SimpleAsyncHTTPClient(max_clients=56, force_instance=True)
        requests = []
        for child, n in self.children:
            child_port = WORKER_TEST_CONTROL_PORTS[n]
            requests.append(HTTPRequest(url='http://[::1]:{port}{path}'.format(port=child_port, path=self.request.uri),
                                        method="POST",
                                        headers=dict(Connection="close", Host="workercontrol"),
                                        body="{}",
                                        request_timeout=10))

        responses = yield [client.fetch(req) for req in requests]
        for (child, n), response in zip(self.children, responses):
            if response.code != 200:
                logging.error("Service process: Couldn't update configs on pid={}".format(child), response=response.body)


class CacheControlHandler(tornado.web.RequestHandler):
    @coroutine
    def get(self):
        yield self.application.update_configs_cache()


class SystemMetricsHandler(tornado.web.RequestHandler):

    def get(self):
        self.write(json_encode({
            'rps': SYSTEM_METRICS.get_avg_rps(),
            'cpu_load': SYSTEM_METRICS.get_avg_cpu_load()
        }))


class MetricsHandler(tornado.web.RequestHandler):

    def get(self):
        self.write(json_encode({'sensors': SOLOMON_METRICS_AGG.get_solomon_sensors()}))


class CacheValue:
    def __init__(self, expired, data):
        self.expired = expired
        self.data = data


# noinspection PyAbstractClass
class ArgusCacheHandler(tornado.web.RequestHandler):
    # noinspection PyMethodOverriding,PyAttributeOutsideInit
    def initialize(self, redis_sentinel):
        self.redis_sentinel = redis_sentinel

    @coroutine
    def get(self):
        key = urlsafe_b64decode(parse_qs(self.request.query)['key'][-1])
        logging.debug("ARGUS: ArgusCacheService: GET", key=key)
        data = None
        cache_get_status = 200
        if self.redis_sentinel is not None and self.redis_sentinel.is_connected():
            try:
                data = yield with_timeout(timedelta(5), self.redis_sentinel.get(key, from_master=True))
            except Exception as e:
                cache_get_status = 500
                logging.error("ARGUS: Failed to get cached value from redis", key=key, exc_message=e.message)
        else:
            cache_get_status = 500
            logging.error("ARGUS: No redis connection")
        if data is None and cache_get_status == 200:
            logging.debug("ARGUS: Key not found", key=key)
            cache_get_status = 404
        if isinstance(data, str):
            data = pickle.loads(data)
            self.write(data)
        else:
            if cache_get_status == 200:
                cache_get_status = 500
            if data is not None:
                logging.error("ARGUS: undefined type", type=type(data), value=data)
        self.set_status(cache_get_status)
        self.finish()

    @coroutine
    def post(self):
        def redis_set_callback(key, redis_reply, post_info):
            logging.info("ARGUS: redis set", key=key, result=redis_reply)
            if redis_reply != "OK":
                post_info["status"] = 500
                logging.error("ARGUS: Redis set error", reply=redis_reply)

        cache_post_info = dict(status=201)
        query_args = parse_qs(self.request.query)
        seconds = int(query_args['seconds'][-1])
        cache_key = urlsafe_b64decode(query_args['key'][-1])
        logging.debug("ARGUS: ArgusCacheService: POST:", key=cache_key)
        cache_value = self.request.body
        if self.redis_sentinel is not None and self.redis_sentinel.is_connected():
            data = pickle.dumps(cache_value)
            self.redis_sentinel.setex_cb(cache_key, seconds, data,
                                         lambda r: redis_set_callback(cache_key, r, cache_post_info))
            logging.debug("ARGUS: ArgusCacheService: Done Redis", status=cache_post_info["status"])
        else:
            cache_post_info["status"] = 500
            logging.error("ARGUS: No redis connection")

        self.set_status(cache_post_info["status"])
        self.finish()


# noinspection PyAbstractClass
class StaticCacheHandler(tornado.web.RequestHandler):
    # noinspection PyMethodOverriding,PyAttributeOutsideInit
    def initialize(self, redis_sentinel):
        self.redis_sentinel = redis_sentinel
        self.metrics = AppMetrics()

    @coroutine
    def get(self):
        start_time = monotonic()
        key = urlsafe_b64decode(parse_qs(self.request.query)['key'][-1])
        data = None
        cached_value = CACHE.get(key)
        expired = False
        cache_get_status = 'miss'
        if cached_value:
            if start_time <= cached_value.expired:
                data = cached_value.data
                cache_get_status = 'hit'
            else:
                CACHE.pop(key, default=None)
                expired = True
        if not data and not expired:
            if self.redis_sentinel is not None and self.redis_sentinel.is_connected():
                try:
                    data = yield with_timeout(REDIS_REQUEST_TIMEOUT_TD, self.redis_sentinel.get(key))
                except TimeoutError:
                    cache_get_status = 'redis_timeout'
                except Exception:
                    logging.exception("Failed to get cached value from redis for key = {}".format(key))
                    cache_get_status = 'redis_error'
            else:
                cache_get_status = 'no_redis'
            if data:
                expired, data = pickle.loads(data)
                CACHE[key] = CacheValue(expired, data)
        if data:
            self.write(data)
        else:
            self.set_status(404)
        self.finish()
        self.metrics.increase(LOCAL_CACHE_GET, monotonic() - start_time, status=cache_get_status)

    @coroutine
    def post(self):
        def redis_set_callback(key, redis_reply):
            logging.info("redis set key='{}' result {}".format(key, redis_reply))

        start_time = monotonic()
        cache_post_status = 'no_put'
        query_args = parse_qs(self.request.query)
        cache_key = urlsafe_b64decode(query_args['key'][-1])
        local_cache_value = CACHE.get(cache_key)
        if local_cache_value is None or local_cache_value.expired < start_time:
            seconds = int(query_args['seconds'][-1])
            expired = start_time + seconds
            cache_value = self.request.body
            CACHE[cache_key] = CacheValue(expired, cache_value)
            cache_post_status = 'put_sent'
            if self.redis_sentinel is not None and self.redis_sentinel.is_connected():
                data = pickle.dumps((expired, cache_value))
                self.redis_sentinel.setex_cb(cache_key, seconds, data, lambda r: redis_set_callback(cache_key, r))
            else:
                cache_post_status = 'no_redis'
        self.set_status(200)
        self.finish()
        self.metrics.increase(LOCAL_CACHE_PUT, monotonic() - start_time, status=cache_post_status)
