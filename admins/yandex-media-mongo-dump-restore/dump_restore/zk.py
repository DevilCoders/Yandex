"""
ZK Lock class
"""
from kazoo.client import KazooClient
from kazoo import exceptions
from socket import getfqdn
import logging


class ZLock:
    def __init__(self, config):
        self.log = logging.getLogger(self.__class__.__name__)
        self.log.info("Found ZK url: %s", config.zk)
        self.zk = KazooClient(config.zk, timeout=10.0)
        if self.zk:
            self.log.info("Connected to zk.")
        else:
            return

        self.zk.start()
        self.path = u'/mongo-dumper/{}'.format(config.db)
        self.zk.ensure_path(self.path)
        self.lock = self.zk.Lock(self.path+'/lock', identifier=getfqdn())

    def acquire(self):
        if not self.zk.connected:
            self.zk.start()

        try:
            is_acquired = self.lock.acquire(blocking=True, timeout=10)
        except exceptions.LockTimeout as fail:
            self.log.debug(fail.message)
            return False

        if not is_acquired:
            return False
        else:
            self.log.debug("Acquire zk lock.")
            return True

    def release(self):
        if not self.zk.connected:
            self.zk.start()
        self.log.debug('Release zk lock.')
        self.lock.release()
