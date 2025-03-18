#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import imp

import sys
import pickle
import subprocess
import logging
import json
import traceback

from antirobot.scripts.log_viewer.app import yt_client
from antirobot.scripts.log_viewer.app import yt_cache
from antirobot.scripts.log_viewer.app import log_mgrs

from yt import wrapper as yt


def DoSearch(ytInst, cacheKey, logId, date, keyType, keyValue, conf):
    tmpDir = os.path.join(conf['cluster_root'], 'tmp')
    yt_client.MkDirSafe(ytInst, tmpDir)

    mgr = log_mgrs.GetPrecalcMgr(logId, keyType)(ytInst, conf['cluster_root'])

    rowNumbers = None
    resTable = ytInst.create_temp_table(path=tmpDir)
    try:
        mgr.SlowSearch(cacheKey, mgr.OriginTable(date), resTable, keyValue)
        ytInst.remove(resTable + '/@expiration_time')

        meta = {'status': 'ready'}
        return meta, resTable
    except:
        print >>sys.stderr, traceback.format_exc()
        meta = {'status': 'error', 'descr': traceback.format_exc()}
        try:
            ytInst.remove(resTable, force=True)
        except:
            pass
        return meta, None


def main():
    logging.getLogger("Yt").setLevel(logging.ERROR)

    (logId, date, keyType, keyValue, conf) = pickle.loads(sys.argv[1])

    ytInst = yt_client.GetYtClient(conf['yt_proxy'], conf['yt_token'])
    cache = yt_cache.YTCache(conf['cluster_root'], ytInst)
    cacheKey = cache.MakeKey(logId, date, keyType, keyValue)
    cache.WriteKey(cacheKey, lambda ytInst: DoSearch(ytInst, cacheKey, logId, date, keyType, keyValue, conf))


if __name__ == "__main__":
    main()
