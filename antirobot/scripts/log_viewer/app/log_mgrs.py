import log_accesslog
import log_market
import log_eventlog


AllMgrs = (
    (log_accesslog.PrecalcMgrIp, 'Accesslog by IP'),
    (log_accesslog.PrecalcMgrYuid, 'Acceesslog by yandexuid'),
    (log_market.PrecalcMgrIp, 'Market accesslog by IP'),
    (log_market.PrecalcMgrYuid, 'Market accesslog by yandexuid'),
    (log_eventlog.PrecalcMgrIp, 'Eventlog by IP'),
    (log_eventlog.PrecalcMgrYuid, 'Eventlog by yandexuid'),
    )


def GetLogIds():
    return (
        log_accesslog.LOG_ID,
        log_market.LOG_ID,
        log_eventlog.LOG_ID
        )

def GetMgrList(logId='all', key='all'):
    res = []
    for mgr, _ in AllMgrs:
        if ((logId == 'all' or mgr.LOG_ID == logId) and
            (key == 'all' or mgr.PRECALC_KEY == key)):
            res.append(mgr)

    return res


def GetPrecalcMgr(logId, keyType):
    mgrs = GetMgrList(logId, keyType)
    if not mgrs:
        raise Exception, "Could not get precalc manager for the specified log name and precalc key"

    if len(mgrs) > 1:
        raise Exception, "More than one precalc module has been found - cannot continue"

    return mgrs[0]
