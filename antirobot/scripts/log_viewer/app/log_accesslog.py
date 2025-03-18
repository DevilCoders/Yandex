import precalc_mgr

LOG_ID = 'yandex-access-log'

class PrecalcMgrIp(precalc_mgr.PrecalcMgr):
    LOG_ID = LOG_ID
    PRECALC_KEY = precalc_mgr.IP_FIELD

    def __init__(self, ytInst, clusterRoot):
        super(PrecalcMgrIp, self).__init__(ytInst, clusterRoot)

    @classmethod
    def _GetMapper(cls, originTable=None, searchFor=None):
        return precalc_mgr.StdAccesslogMap(cls.PRECALC_KEY, value=searchFor, originTable=originTable)


class PrecalcMgrYuid(precalc_mgr.PrecalcMgr):
    LOG_ID = LOG_ID
    PRECALC_KEY = precalc_mgr.YUID_FIELD

    def __init__(self, ytInst, clusterRoot):
        super(PrecalcMgrYuid, self).__init__(ytInst, clusterRoot)

    @classmethod
    def _GetMapper(cls, originTable=None, searchFor=None):
        return precalc_mgr.StdAccesslogMap(cls.PRECALC_KEY, value=searchFor, originTable=originTable)

