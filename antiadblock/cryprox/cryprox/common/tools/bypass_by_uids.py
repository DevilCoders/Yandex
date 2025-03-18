import os
from monotonic import monotonic

import numpy as np

from antiadblock.cryprox.cryprox.service.jslogger import JSLogger
from antiadblock.cryprox.cryprox.config.service import BYPASS_UIDS_FILE_PATH
from antiadblock.cryprox.cryprox.config.system import BYPASS_UIDS_TYPES

logger = JSLogger('root')


class Singleton(type):
    _instances = {}

    def __call__(cls, *args, **kwargs):
        if cls not in cls._instances:
            cls._instances[cls] = super(Singleton, cls).__call__(*args, **kwargs)
        return cls._instances[cls]


class BypassByUids:
    __metaclass__ = Singleton

    def __init__(self):
        self.uids = dict()
        self.last_update_inode = dict()

    def update_uids(self):
        for device in BYPASS_UIDS_TYPES:
            filename = os.path.join(BYPASS_UIDS_FILE_PATH, BYPASS_UIDS_TYPES[device])
            if os.path.exists(filename):
                inode = os.stat(filename).st_ino
                if inode != self.last_update_inode.get(device):
                    try:
                        new = np.memmap(filename, dtype=np.uint64, mode='r')
                        self.uids[device] = new
                        self.last_update_inode[device] = inode
                        logger.info(None, action='bypass_uids_update', status='success', device=device, uids_filename=filename)
                    except Exception:
                        self.uids[device] = None
                        self.last_update_inode[device] = None
                        logger.error(None, action='bypass_uids_update', status='fail', device=device, exc_info=True, uids_filename=filename)

    def has(self, device, uid, metrics=None):
        start = monotonic()
        result = False
        uids = self.uids.get(device)
        if uid is not None and uids is not None:
            try:
                uid = np.uint64(uid)
            except ValueError:
                logger.error(None, action='check_uid_is_bypass', device=device, uid=uid, exc_info=True)
            else:
                if uids[0] <= uid <= uids[-1]:
                    result = uids[np.searchsorted(uids, uid)] == uid  # np.searchsorted(a, v) returns i such that: a[i-1] < v <= a[i]
        if metrics is not None:
            metrics.increase('bypass_uid', duration=int((monotonic() - start) * 1e6), bypass=int(result))
        return result
