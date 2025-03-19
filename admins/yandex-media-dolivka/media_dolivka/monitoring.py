#!/usr/bin/python3
"""
Monitoring class using zookeeper in dolivka
"""
from socket import getfqdn
from re import match
import logging
import requests
from kazoo.client import KazooClient
from kazoo.client import KeeperState
from kazoo.client import KazooState
from kazoo.client import KazooRetry
from kazoo import exceptions


def get_yaenv():
    """
    just get yandex-env
    :return: env
    """
    try:
        envfile = open('/etc/yandex/environment.type', 'r')
    except IOError:
        return 'testing'
    return envfile.read().split()[0]


def get_zk_hosts(group):
    """
    resolve group into zk hosts
    :param group: conductor group
    :return: host list in KazooClient format
    """
    c_api_url = "https://c.yandex-team.ru/api-cached/groups2hosts/"
    try:
        data = requests.get(c_api_url + group)
    except (requests.ConnectionError, requests.HTTPError) as err:
        print(err)
        return "zk01e.media.yandex.net:2181,zk01f.media.yandex.net:2181,zk01h.media.yandex.net:2181"

    return data.content.decode("utf-8").replace('\n', ':2181,')[:-1]


class Monitor:  # pylint: disable=old-style-class
    """
    Save and get monitoring statuses from ZK
    """
    def __init__(self, config):
        self.crit_thr = config.crit_thr
        self.warn_thr = config.warn_thr
        self.zk_root = config.zk_root + '/' + getfqdn()
        self.log = logging.getLogger("Monitor")

        self.zk_timeout = 10000
        self.status_path = self.zk_root + '/status'

        env = get_yaenv()
        if env == 'testing':
            group = 'media-test-zk'
        else:
            group = 'media-stable-zk'

        self.log.debug(config.toDict().__str__())
        zk_hosts = get_zk_hosts(group)
        zk_conn_retry = KazooRetry(max_tries=-1, max_delay=10)
        zk_cmd_retry = KazooRetry(max_tries=10, max_delay=10)
        msg = "Connected to zk with hosts: %s, timeout: %s" % (zk_hosts, self.zk_timeout)
        self.log.debug(msg)
        self.zoo = KazooClient(
            hosts=zk_hosts,
            timeout=self.zk_timeout,
            connection_retry=zk_conn_retry,
            command_retry=zk_cmd_retry
        )
        self.zoo.add_listener(self.zk_listener)
        self.connect()

    def zk_listener(self, state):
        if state == KazooState.LOST:
            self.log.debug("zookeeper connection lost.")
        elif state == KazooState.SUSPENDED:
            self.log.debug("zookeeper connection closed")
        else:
            self.log.debug("zookeeper connection established.")

    def connect(self):
        """
        just open zk session
        :return: bool
        """
        try:
            self.zoo.start(self.zk_timeout)
            self.log.debug("Connected to zk.")
            return True
        except (exceptions.ConnectionLoss, exceptions.KazooException):
            self.log.error('The zookeeper is unreachable')
            return False

    def disconnect(self):
        """
        just close zk session
        :return: None
        """
        if self.zoo.client_state == KeeperState.CONNECTED:
            self.zoo.stop()
            self.zoo.close()

    def get_status(self):
        """
        get failed count from zk
        :return: zk data with errors count
        """
        logging.getLogger("Monitor.get_status")
        try:
            is_exists = self.zoo.exists(self.status_path)
        except (exceptions.ConnectionLoss, exceptions.KazooException) as error:
            self.log.info(error)
            return "-1"

        if is_exists:
            try:
                data, meta = self.zoo.get(self.status_path)
            except (exceptions.ConnectionLoss, exceptions.KazooException) as error:
                self.log.error('Failed to get data from zk')
                self.log.error(error)
                data = None
        else:
            msg = 'Monitoring status %s not found in zk' % self.status_path
            self.log.error(msg)
            data = None

        if data:
            value = data.decode("utf-8")
            if not match(r'\d+', value):
                value = "-1"
        else:
            value = "-1"

        return value

    def set_status(self, state):
        """
        set errors count (for now just a number)
        :param state: state - count
        :return: bool true or false
        """
        logging.getLogger("Monitor.set_status")
        msg = 'Set status %s' % state
        self.log.debug(msg)
        data = bytes(state.encode())
        try:
            is_exists = self.zoo.exists(self.status_path)
        except (exceptions.ConnectionLoss, exceptions.KazooException) as error:
            self.log.info(error)
            is_exists = False

        if not is_exists:
            self.zoo.ensure_path(self.zk_root)
            try:
                self.zoo.create(self.status_path, data)
                msg = 'Created zk node for %s monitoring' % self.status_path
                self.log.info(msg)
            except (exceptions.ConnectionLoss, exceptions.KazooException) as error:
                msg = 'Filed to set status %s in zk. Error: %s' % (state, error)
                self.log.error(msg)
                return False
        else:
            msg = 'Set monitoring status in %s' % self.status_path
            self.log.info(msg)
            try:
                self.zoo.set(self.status_path, data)
            except (exceptions.ConnectionLoss, exceptions.KazooException) as error:
                msg = 'Filed to set status {} in zk. Error: {}'.format(state, error)
                self.log.error(msg)
                return False

        return True

    def inc_state(self):
        """
        increment fails in zk status
        :return: none
        """
        logging.getLogger("Monitor.inc_state")
        data = self.get_status()
        if data == '-1':
            self.log.error('Failed to get status')

        if data:
            status = int(data)
            status += 1
        else:
            status = 0

        try:
            is_exists = self.zoo.exists(self.status_path)
        except (exceptions.ConnectionLoss, exceptions.KazooException) as error:
            self.log.info(error)
            is_exists = False

        if not is_exists:
            self.zoo.ensure_path(self.zk_root)
            try:
                self.zoo.create(self.status_path, bytes(status))
            except (exceptions.ConnectionLoss, exceptions.KazooException):
                self.log.error('Failed to create increment monitoring status')
        else:
            data = bytes(str(status).encode())
            try:
                self.zoo.set(self.status_path, data)
            except (exceptions.ConnectionLoss, exceptions.KazooException):
                self.log.error('Failed to increment existing monitoring status')
