"""
Lock module
"""
import os
import time
import logging
import dotmap
from kazoo.exceptions import NoNodeError, BadVersionError, LockTimeout
from .utils import str2time, backup_cname
from .constants import HOSTNAME, LOCK_FLAG


class DummyLock(object):
    """
    Simple dummy lock
    """
    def __init__(self):
        pass

    @staticmethod
    def acquire():
        """dummy acquire"""
        return True

    @staticmethod
    def release():
        """dummy release"""

class Lock(object):
    """
    Production quality zk lock
    """
    def __init__(self, zk_conn, conf):
        self.log = logging.getLogger(self.__class__.__name__)

        self.conf = conf
        self.period = str2time(conf.backup_period or "2.1h")

        self.zk_conn = zk_conn
        self.path = os.path.join("backup", conf.lock or backup_cname(), "lock")

        self.last_ts_path = self.path + "-last_timestamp"
        self._lock = zk_conn.Lock(self.path, identifier=HOSTNAME)
        self.log.debug("Create lock: '%s'", self.path)

    def acquire(self):
        "acquire zk lock"
        try:
            if not self._lock.acquire(blocking=True, timeout=self.conf.timeout):
                return False
            stats = dotmap.DotMap({"version": -1})
            try:
                (lasttime, stats) = self.zk_conn.get(self.last_ts_path)
                lasttime = float(lasttime)
            except (ValueError, NoNodeError):
                lasttime = time.time() - self.period

            if time.time() - lasttime < self.period:
                self.log.debug(
                    "The last backup was still relevant (backup period %s)",
                    self.conf.backup_period
                )
                return False
            self.zk_conn.ensure_path(self.last_ts_path)
            self.zk_conn.set(self.last_ts_path, str(time.time()), stats.version)
        except (BadVersionError, LockTimeout):
            return False

        with open(LOCK_FLAG, "w"):
            self.log.debug("touch %s flag on success zk lock", LOCK_FLAG)

        return True

    def release(self):
        "release zk lock"
        self._lock.release()
