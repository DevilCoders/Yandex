#!/usr/bin/env python
# coding=utf-8
import os
import logging
import sys
import errno
import signal
import time
from functools import partial
from multiprocessing import Queue as MPQueue
from multiprocessing.dummy import JoinableQueue as Queue
from random import Random

from tornado.ioloop import IOLoop, PeriodicCallback
from tornado.util import errno_from_exception
from tornado.gen import coroutine
from tornado.netutil import Resolver
from antiadblock.cryprox.cryprox.service.resolver import CachedResolver
from antiadblock.cryprox.cryprox.service.pipeline import Pipeline, EventType
from antiadblock.cryprox.cryprox.service.metrics import AppMetrics


def fork(num_processes):
    """
    fork() process `num_processes` times.
    :param num_processes: Count of forks. If num_processes <= 0, then num_processes = cpu_count().
    :return:
    [], [process_metrcis_queue], worker_index  -  for child process
    [(pid, worker_index) for all children], [all metrics queues], None  -  for master process

    If error occour os.fork() raise exception (see os.fork() documentation).
    """
    from os import fork as _fork
    from tornado.process import cpu_count

    if num_processes is None or num_processes <= 0:
        num_processes = cpu_count()

    if IOLoop.initialized():
        raise RuntimeError("Cannot run in multiple processes: IOLoop instance "
                           "has already been initialized. You cannot call "
                           "IOLoop.instance() before calling start_processes()")

    childs = list()
    queues = list([Queue()])
    for i in range(num_processes):
        queue = MPQueue()
        pid = _fork()
        if pid == 0:
            return [], Pipeline([queue]), i
        queues.append(queue)
        childs.append((pid, i))
    return childs, Pipeline(queues), None


def set_signals_handler(func):
    """
    Set `func` as terminate signals handler.
    :param func: Handler function.
    """

    signal.signal(signal.SIGQUIT, func)
    signal.signal(signal.SIGTERM, func)
    signal.signal(signal.SIGINT, func)
    # обрабатываем завершение дочерних процессов
    signal.signal(signal.SIGCHLD, wait_child)


def signal_handler(signum, *_):
    """
    Signal handler. Stop io_loop.
    :param signum: Signal number.
    :param frame: Current stack frame.
    """
    logging.debug(u'process {pid} got signal {signum}'.format(pid=os.getpid(), signum=signum))
    loop = IOLoop.current()
    loop.stop()


def wait_child(signum, *_):
    try:
        pid, status = os.wait()
    except OSError as e:
        if errno_from_exception(e) == errno.EINTR:
            return
        raise

    if os.WIFSIGNALED(status):
        logging.warning(u'child (pid {}) killed by signal {}'.format(pid, os.WTERMSIG(status)), action='kill')
    elif os.WEXITSTATUS(status) != 0:
        logging.warning(u'child (pid {}) exited with status {}'.format(pid, os.WEXITSTATUS(status)), action='kill')
    else:
        logging.warning(u'child (pid {}) exited normally'.format(pid), action='kill')
    logging.warning(u'child pid={} exited, start parent exit'.format(pid), action='kill')
    try:
        IOLoop.current().stop()
    finally:
        sys.exit(1)


def on_parent_death(signum, *_):
    logging.warning('parent died, starting child exit', action='kill')
    try:
        time.sleep(60)
    finally:
        sys.exit(1)


def main():
    """
    Set signal handlers.
    Read configs and binding a sockets.
    Forks.
    Process requests.
    """
    from tornado.netutil import bind_sockets
    from tornado.httpserver import HTTPServer

    from antiadblock.cryprox.cryprox.config import service as service_config
    from antiadblock.cryprox.cryprox.service.service import make_worker_app, make_service_app

    from antiadblock.cryprox.cryprox.common.config_utils import get_configs, get_tvm_client
    from antiadblock.cryprox.cryprox.common.tools.bypass_by_uids import BypassByUids
    from antiadblock.cryprox.cryprox.common.tools.internal_experiments import InternalExperiment

    logging.getLogger().setLevel(service_config.LOGGING_LEVEL)
    logging.getLogger('tornado.access').disabled = True
    # logging.getLogger('tornado.application').disabled = True
    # logging.getLogger('tornado.general').disabled = True

    set_signals_handler(signal_handler)

    children, pipeline, worker_n = fork(service_config.WORKERS_COUNT)

    # do not start ioloop before fork
    # http://y.tsutsumi.io/keyerror-in-self_handlers-a-journey-deep-into-tornados-internals.html
    if children:  # Assume we are a parent (cause we have children, meh)
        io_loop = IOLoop.current()
        service_socket = bind_sockets(port=service_config.SERVICE_PORT, address=service_config.HOST, reuse_port=True)
        from antiadblock.cryprox.cryprox.config.service import ENV_TYPE
        if ENV_TYPE in ['testing', 'development']:
            from antiadblock.libs.tornado_redis.lib.test_sentinel import FakeRedisSentinel as RedisSentinel
        else:
            from antiadblock.libs.tornado_redis.lib.sentinel import RedisSentinel

        redis_sentinel = RedisSentinel(
            sentinels=[(h, service_config.REDIS_SENTINEL_PORT) for h in service_config.REDIS_HOSTS],
            service_name=service_config.REDIS_SERVICE_NAME,
            password=service_config.REDIS_PASSWORD,
            timeout=service_config.REDIS_CONNECT_TIMEOUT,
            io_loop=io_loop,
            update_period=service_config.REDIS_SENTINEL_UPDATE_PERIOD,
            debug=service_config.LOGGING_LEVEL <= logging.DEBUG,
        )
        io_loop.spawn_callback(redis_sentinel.connect)

        # init tvm_client after fork() https://st.yandex-team.ru/PASSP-21983
        tvm_client = get_tvm_client()
        configs = io_loop.run_sync(partial(get_configs, service_config.CONFIGSAPI_URL, tvm_client))
        app = make_service_app(children, tvm_client, redis_sentinel)
        io_loop.run_sync(partial(app.update_configs_cache, {key: {'config': value.to_dict(), 'version': value.version, 'statuses': ['active']} for key, value in configs.iteritems()}))
        io_loop.run_sync(app.update_script_detect_cache)

        server = HTTPServer(app)
        server.add_sockets(service_socket)

        @coroutine
        def _reload_configs_cache():
            logging.debug(u"Reloading configs cache in pid: {pid}".format(pid=os.getpid()))
            yield app.update_configs_cache()

        logging.debug(u"Workers initiated", pids=children)
        metrics_container = list()
        pipeline.register_event_listener(EventType.SOLOMON_METRICS, lambda data: metrics_container.append(data))
        pipeline.register_event_listener(EventType.APPEND_REQUEST, app.process_system_metrics)
        PeriodicCallback(app.update_configs_cache, service_config.CONFIG_UPDATE_PERIOD).start()  # Every 60 seconds
        PeriodicCallback(partial(app.aggr_metrics_from_list, metrics_container), service_config.METRICS_TRANSFER_DELAY * 1000).start()
        io_loop.spawn_callback(partial(app.start_pipeline_loop, pipeline))
        io_loop.spawn_callback(app.measure_cpu)

        # в тестовом окружении в самих тестах мы переопределяем handler для отдачи контента stub-сервером
        # в итоге закешироваться может все что угодно, но не скрипт детекта
        # поэтому в тестовом окружении кеш не обновляем
        if ENV_TYPE != 'testing':
            PeriodicCallback(app.update_script_detect_cache, service_config.SCRIPT_DETECT_UPDATE_PERIOD).start()  # Every 60 seconds

    else:
        # local import here because we need this import to execute after sys.path.append in main
        from antiadblock.cryprox.cryprox.config.service import ENV_TYPE, WORKER_TEST_CONTROL_PORTS, \
            LOG_PATH, CALLGRIND_FILENAME, PROFILING, CALLGRIND_STATS_UPDATE_PERIOD, YAPPI_CLOCK_TYPE
        import yappi

        if sys.platform.startswith('linux'):
            import prctl

            prctl.set_pdeathsig(signal.SIGUSR1)
            signal.signal(signal.SIGUSR1, on_parent_death)

        io_loop = IOLoop.current()
        worker_socket = bind_sockets(port=service_config.WORKER_PORT, address=service_config.HOST, reuse_port=True)
        while True:
            try:
                configs = io_loop.run_sync(partial(get_configs, service_config.CONFIGSAPI_CACHE_URL))
                break
            except Exception as e:
                logging.error(u"Failed attempt to get configs in pid: {pid}, error: {error}".format(pid=os.getpid(), error=str(e)))
                continue

        if PROFILING:
            yappi.set_clock_type(YAPPI_CLOCK_TYPE)
            yappi.start()

        app = make_worker_app(configs, pipeline)
        server = HTTPServer(app, decompress_request=True)
        server.add_sockets(worker_socket)

        @coroutine
        def save_callgrind_stats():
            if yappi.is_running():
                filename = os.path.join(LOG_PATH, CALLGRIND_FILENAME)
                try:
                    yappi.get_func_stats().save(filename, type='callgrind')
                except Exception as e:
                    logging.error(u"Error while saving callgrind stats in {filename}, error: {error}".format(filename=filename, error=str(e)))

        if ENV_TYPE in ['testing', 'development']:
            test_control_sockets = bind_sockets(port=WORKER_TEST_CONTROL_PORTS[worker_n], address="::1")
            server.add_sockets(test_control_sockets)

        half_shift = service_config.CONFIG_UPDATE_PERIOD / 20  # assume this time is not critical
        random_shift = 2 * Random(os.getpid()).random() * half_shift - half_shift  # to break synchronous config update led to service unavailable
        PeriodicCallback(app.update_configs, service_config.CONFIG_UPDATE_PERIOD + random_shift).start()  # Every 60 seconds +-
        PeriodicCallback(BypassByUids().update_uids, service_config.BYPASS_UIDS_UPDATE_PERIOD).start()
        PeriodicCallback(InternalExperiment().update_config, service_config.INTERNAL_EXPERIMENT_CONFIG_UPDATE_PERIOD).start()
        if PROFILING:
            PeriodicCallback(save_callgrind_stats, CALLGRIND_STATS_UPDATE_PERIOD).start()

    PeriodicCallback(partial(AppMetrics.send_metrics_by_queue, pipeline), service_config.METRICS_TRANSFER_DELAY * 1000).start()
    Resolver.configure(CachedResolver)
    io_loop.start()
    io_loop.close(all_fds=True)


if __name__ == '__main__':
    sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
    main()
