"""
MongoDB control
"""

import logging
import pathlib

from tenacity import retry, retry_if_exception_type, stop_after_attempt

from mdb_mongo_tools.mongodb import DATADIR_ROOT
from mdb_mongo_tools.service_ctl import CmdCtl, get_service_ctl_instance
from mdb_mongo_tools.util import purge_dir_contents


class MongodCtl:
    """
    mongod control class
    """

    def __init__(self, conf: dict, dbpath: str):
        self._wd_stop_file = conf['wd_stop_file']
        self._dbpath = dbpath
        self._ctl = get_service_ctl_instance(conf['service_ctl'])

    def start(self):
        """
        Start mongodb instance
        """
        logging.info('Starting mongod')
        self._ctl.command(service='mongodb', cmd=CmdCtl.START)

    def stop(self):
        """
        Stop mongodb instance
        """
        logging.info('Stopping mongod')
        self._ctl.command(service='mongodb', cmd=CmdCtl.STOP)

    @retry(retry=retry_if_exception_type(OSError), stop=stop_after_attempt(5))
    def purge_dbpath(self, force=False):
        """
        Purge datadir path
        """
        if not force:
            pass  # TODO: check if service is running

        assert self._dbpath.startswith(DATADIR_ROOT)
        logging.debug('Removing mongodb data dir: %s', self._dbpath)
        purge_dir_contents(self._dbpath)

    def disable_watchdog(self):
        """
        Set watchdog stop flag
        """
        logging.info('Disabling mongod watchdog')
        pathlib.Path(self._wd_stop_file).touch(exist_ok=True)

    def enable_watchdog(self):
        """
        Delete watchdog stop flag
        """
        logging.info('Enabling mongod watchdog')
        pathlib.Path(self._wd_stop_file).unlink()
