"""
Resetup module
"""

import logging
import time
from abc import ABC, abstractmethod
from datetime import datetime, timedelta
from typing import Dict, Type

from mdb_mongo_tools.mongo_ctl import MongodCtl
from mdb_mongo_tools.mongodb import MongoConnection


class BaseResetup(ABC):
    """
    Base class for resetup
    """

    def __init__(self, conf: dict, conn: MongoConnection, ctl: MongodCtl):
        self._conf = conf
        self._conn = conn
        self._ctl = ctl
        self._start_time = None
        self._end_time = None

    def start(self):
        """
        Start wrapper
        """
        self._start_time = datetime.now()
        return self._start()

    def fake_start(self):
        """
        Imitate starting wrapper
        """
        self._start_time = datetime.now()

    @abstractmethod
    def _start(self):
        """
        Start procedure
        """

    def wait(self):
        """
        Wait wrapper
        """
        result = self._wait()
        self._end_time = datetime.now()
        return result

    @abstractmethod
    def _wait(self):
        """
        Start procedure
        """

    @property
    def duration(self) -> timedelta:
        """
        Get resetup duration
        """
        if not self._start_time:
            return timedelta(0)
        end_time = self._end_time or datetime.now()
        return end_time - self._start_time


class InitialSyncResetup(BaseResetup):
    """
    Resetup via initial sync
    """

    def _start(self):
        """
        Drop datadir and start initial sync
        """
        self._ctl.disable_watchdog()
        self._ctl.stop()
        self._ctl.purge_dbpath()
        self._ctl.enable_watchdog()
        self._ctl.start()

    def _wait(self):
        """
        Wait until resync is finished
        """
        check_delay = timedelta(**self._conf['status_check_delay']).seconds
        wait_until = datetime.now() + timedelta(**self._conf['timeout'])
        while datetime.now() < wait_until:
            if self._conn.is_readable:
                logging.debug('mongod is readable, initial sync completed')
                return

            logging.debug('mongod still is not readable after %s of resetup, sleep for %s', self.duration, check_delay)
            time.sleep(check_delay)


def get_resetup_instance(rtype, *args, **kwargs) -> BaseResetup:
    """
    Return class instance for required type of sender
    """
    impl: Dict[str, Type[BaseResetup]] = {
        'initial_sync': InitialSyncResetup,
    }
    db_class = impl.get(rtype, None)

    if db_class:
        return db_class(*args, **kwargs)

    raise NotImplementedError('Unknown instance type: {rtype}'.format(rtype=rtype))
