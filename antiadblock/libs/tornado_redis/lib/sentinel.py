from time import time
import socket
import logging
import datetime
import random
from functools import partial
from abc import ABCMeta, abstractmethod

from enum import IntEnum
from tornado import gen
from tornado.ioloop import IOLoop
import toredis

from dc import group_by_preferred_dc_for_current_host

logger = logging.getLogger(__name__)


class Ipv6RedisClient(toredis.Client):
    def __init__(self, io_loop=None, timeout=1.0, disconnect_callback=None):
        super(Ipv6RedisClient, self).__init__(io_loop=io_loop)
        self.timeout = timeout
        self.timeout_td = datetime.timedelta(seconds=self.timeout)
        self._disconnect_callback = disconnect_callback
        self.available = False
        self.host = None
        self.port = None

    def on_disconnect(self):
        self.available = False
        if self._disconnect_callback is not None:
            self._disconnect_callback()

    def __assure_client_closed(self):
        if self.is_connected():
            try:
                self.available = False
                self.close()
            except Exception:
                logger.warning('Failed to close connected client {}'.format(self))

    def connect(self, host='localhost', port=6379, callback=None):
        def handle_timeout():
            """ Connection timed out """
            callback(False)
            if self._stream:
                self._stream.close()
            sock.close()

        def handle_connected():
            """ Connection is established """
            self._io_loop.remove_timeout(timeout_handle)
            callback(True)

        self.__assure_client_closed()
        self.host = host
        self.port = port
        timeout_handle = self._io_loop.add_timeout(self.timeout_td, handle_timeout)
        sock = socket.socket(socket.AF_INET6, socket.SOCK_STREAM, 0)
        return self._connect(sock, (host, port), handle_connected)

    def __repr__(self):
        return '{}({}:{})'.format(Ipv6RedisClient.__name__, self.host, self.port)


class SentinelState(IntEnum):
    CONNECTED = 0
    IN_PROGRESS = 1
    NEW = 2
    FAILED_SLAVE = 3
    FAILED_SENTINEL = 4


class RedisSentinelInterface:  # to support it in FakeRedisSentinel
    __metaclass__ = ABCMeta

    @abstractmethod
    def is_connected(self):
        raise NotImplementedError

    @abstractmethod
    def connect(self):
        raise NotImplementedError

    @abstractmethod
    def get_cb(self, key, callback):
        raise NotImplementedError

    @abstractmethod
    def get(self, key):
        raise NotImplementedError

    @abstractmethod
    def setex_cb(self, key, seconds, value, callback):
        raise NotImplementedError

    @abstractmethod
    def setex(self, key, seconds, value):
        raise NotImplementedError


class RedisSentinel(RedisSentinelInterface):

    def __init__(self, sentinels, service_name, password=None, timeout=1.0, io_loop=None, update_period=60, debug=False):
        self.sentinels = sentinels
        self.service_name = service_name
        self.password = password
        self._io_loop = io_loop or IOLoop.instance()
        if debug:
            logger.setLevel(logging.DEBUG)

        self.sentinel = Ipv6RedisClient(self._io_loop, timeout)
        self.master = Ipv6RedisClient(self._io_loop, timeout)
        self.slave = Ipv6RedisClient(self._io_loop, timeout)
        self.discovered_master = None
        self.discovered_slaves = None
        self.discovered_slave = None
        self.current_slave = self.current_sentinel = -1
        self.update_period = update_period

        self.state = SentinelState.NEW

    def is_connected(self):
        return self.sentinel.available and self.master.available and self.slave.available

    def connect(self):
        logger.debug('New connection to RedisSentinel. Sentinels: {}'.format(self.sentinels))
        self._io_loop.spawn_callback(self.__update_sentinel_state_loop)

    def __connect(self):
        logger.debug('New connection to SENTINEL')
        self.current_slave = self.current_sentinel = -1
        self.sentinel.available = self.master.available = self.slave.available = False
        self.__connect_to_next_sentinel()

    @gen.coroutine
    def __update_sentinel_state_loop(self):
        in_progress_start_time = 0
        while True:
            yield gen.sleep(self.update_period)
            if self.state == SentinelState.NEW:
                self.state = SentinelState.IN_PROGRESS
                in_progress_start_time = time()
                self.__connect()
            elif self.state == SentinelState.FAILED_SENTINEL:
                self.state = SentinelState.IN_PROGRESS
                in_progress_start_time = time()
                self.__connect_to_next_sentinel()
            elif self.state == SentinelState.FAILED_SLAVE:
                self.state = SentinelState.IN_PROGRESS
                in_progress_start_time = time()
                self.__connect_to_next_slave()
            elif self.state == SentinelState.CONNECTED and self.is_connected():
                # check if MASTER addr changed:
                self.sentinel.send_message(['sentinel', 'get-master-addr-by-name', self.service_name], self.__on_get_master_address)
                # check if there are better slave available:
                self.sentinel.send_message(['sentinel', 'slaves', self.service_name], self.__on_get_slaves)
                yield gen.sleep(self.update_period)
            elif (self.state == SentinelState.CONNECTED and not self.is_connected()) or (self.state == SentinelState.IN_PROGRESS and time() - in_progress_start_time > self.update_period):
                self.state = SentinelState.NEW
            yield

    def __set_sentinel_failed(self):
        self.sentinel.available = False
        self.state = SentinelState.FAILED_SENTINEL

    def __connect_to_next_sentinel(self):
        if self.current_sentinel + 1 < len(self.sentinels):
            self.current_sentinel += 1
            host, port = self.sentinels[self.current_sentinel]
            self.sentinel.connect(host, port, callback=self.__on_connection_to_sentinel)
        else:
            self.state = SentinelState.NEW
            logger.error('Failed connect to all SENTINELS {}. Starting again.'.format(self.sentinels))

    def __connect_to_next_slave(self):
        if self.current_slave + 1 < len(self.discovered_slaves):
            self.current_slave += 1
            self.discovered_slave = self.discovered_slaves[self.current_slave]
            host, port = self.discovered_slave
            self.slave.connect(host, port, callback=partial(self.__on_connection_to_data_node, self.slave, state_on_failure=SentinelState.FAILED_SLAVE))
        else:
            logger.error('Failed to connect all SLAVES {} from SENTINEL {}'.format(self.discovered_slaves, self.sentinel))
            self.slave.available = False
            self.__set_sentinel_failed()

    def __on_connection_to_sentinel(self, status):
        if status and self.sentinel.is_connected():
            self.sentinel.available = True
            self.sentinel.send_message(['sentinel', 'get-master-addr-by-name', self.service_name], self.__on_get_master_address)
            self.sentinel.send_message(['sentinel', 'slaves', self.service_name], self.__on_get_slaves)
        else:
            logger.error('failed connect to SENTINEL {}'.format(self.sentinels[self.current_sentinel]))
            self.__set_sentinel_failed()

    def __on_get_master_address(self, *args):
        # expecting: ['2a02:6b8:c0e:71c:0:1589:5483:4ee0', '6379']
        if args[0] is None or args[0] == '-IDONTKNOW':
            logger.error('Failed get MASTER from SENTINEL {}'.format(self.sentinel))
            self.__set_sentinel_failed()
            return

        ip, port = args[0]
        port = int(port)
        try:
            host = socket.gethostbyaddr(ip)[0]
            discovered_master = (host, port)
        except Exception:
            logger.error('failed to rdns master ip: {}'.format(ip))
            discovered_master = (ip, port)
        if self.discovered_master != discovered_master or not self.master.available:
            self.discovered_master = discovered_master
            self.master.connect(discovered_master[0], discovered_master[1], callback=partial(self.__on_connection_to_data_node, self.master, state_on_failure=SentinelState.FAILED_SENTINEL))

    def __on_get_slaves(self, *args):
        slaves = args[0]
        if isinstance(slaves, Exception):
            logger.error('Failed get SLAVES from SENTINEL {}. Exception: {}'.format(self.sentinel, slaves))
            self.__set_sentinel_failed()
            return
        discovered_slaves = []
        if slaves is not None:
            for slave in slaves:
                ip = slave[3]
                port = int(slave[5])
                try:
                    host = socket.gethostbyaddr(ip)[0]
                    discovered_slaves.append((host, port))
                except Exception:
                    logger.error('failed to rdns ip: {}'.format(ip))
        discovered_slaves.append(self.discovered_master)
        grouped_slaves = group_by_preferred_dc_for_current_host(discovered_slaves)
        preferable_slaves = grouped_slaves[0]
        if self.discovered_slave not in preferable_slaves or not self.slave.available:
            map(random.shuffle, grouped_slaves)
            self.discovered_slaves = sum(grouped_slaves, [])
            self.current_slave = -1
            self.__connect_to_next_slave()

    def __on_connection_to_data_node(self, client, status, state_on_failure=None):
        if status:
            logger.debug('{} connected with status {}'.format(client, status))
            client.auth(self.password, callback=partial(self.__on_auth, client))
        else:
            logger.error('Failed connect to {} ({}, {})'.format(client, client.host, client.port))
            client.available = False
            if state_on_failure is not None:
                # if current state worse than `state_on_failure` - we won't change it
                # max(SentinelState.FAILED_SENTINEL, SentinelState.FAILED_SLAVE) -> <SentinelState.FAILED_SENTINEL: 4>
                self.state = max(self.state, state_on_failure)

    def __on_auth(self, client, *args):
        status = args[0]
        if isinstance(status, Exception):
            logger.error('{} AUTH failed {}'.format(client, status))
            self.__set_sentinel_failed()
            return
        logger.debug('{} AUTH successfull'.format(client))
        client.available = True
        if self.is_connected():
            self.state = SentinelState.CONNECTED

    def get_cb(self, key, callback):
        self.slave.get(key, callback)

    def get_from_master(self, key, callback):
        self.master.get(key, callback)

    @gen.coroutine
    def get(self, key, from_master=False):
        if from_master:
            response = yield gen.Task(self.get_from_master, key)
        else:
            response = yield gen.Task(self.get_cb, key)
        raise gen.Return(response)

    def setex_cb(self, key, seconds, value, callback):
        self.master.setex(key, seconds, value, callback)

    @gen.coroutine
    def setex(self, key, seconds, value):
        response = yield gen.Task(self.setex_cb, key, seconds, value)
        raise gen.Return(response)
