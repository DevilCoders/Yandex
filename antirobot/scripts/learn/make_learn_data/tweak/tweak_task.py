import os
import sys
import subprocess
import time
from collections import namedtuple

import yt.wrapper as yt

DEFAULT_TASK_WEIGHT = 1

class TweakTask:
    NAME = 'TweakTask'

    SIMULATE = os.environ.get('SIMULATE')
    SIMULATE_COUNT = 100000
    NeedPrepare = False
    Weight = DEFAULT_TASK_WEIGHT

    def __init__(self):
        pass

    def NamePrefix(self):
        return self.NAME + ':'

    def PrintNamePrefix(self):
        print >>sys.stderr, self.NamePrefix(),

    def RunMap(self, *args, **kwargs):
        self.PrintNamePrefix()
        yt.run_map(*args, spec={"data_size_per_job": 1024 * 1024 * 1024, "enable_output_table_index": True}, format=yt.YsonFormat(control_attributes_mode="row_fields"), **kwargs)

    def RunReduce(self, *args, **kwargs):
        self.PrintNamePrefix()
        yt.run_reduce(*args, format=yt.YsonFormat(control_attributes_mode="row_fields"), **kwargs)

    def RunSort(self, table, key):
        yt.run_sort(table, sort_by=key)

    def ReadTable(self, table):
         return yt.read_table(table)

    def YtMkDir(self, dirName):
        if not yt.exists(dirName):
            yt.mkdir(dirName, recursive=True)

    def YtDropDir(self, dirName):
        yt.remove(dirName, recursive=True, force=True)

    def Trace(self, msg):
        print >>sys.stderr, time.strftime('%Y.%m.%d %H:%M:%S'),
        self.PrintNamePrefix()
        print >>sys.stderr, msg

    def UpdateRndReqFull(self, rndReqFullIter):
        """ calledbefore Prepare()
            rndReqFullIter is an iterator to rnd_request.RndReqFull
        """
        pass

    # Prepare data, then save it to outDir
    # if always is true, don't check already prepared data
    def Prepare(self, outDir, always=False):
        raise Exception, 'Method not implemented'

    def LoadPrepared(self, preparedDataDir):
        pass

    def IsSuspicious(self, reqid):
        return False

    # Must return SuspInfo() or None
    def SuspiciousInfo(self, reqid):
        return None

    def PrintStat(self):
        pass
