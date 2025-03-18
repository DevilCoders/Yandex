import os
import sys
import datetime
import argparse
import logging


import yt.wrapper as yt

from antirobot.scripts.log_viewer.app import yt_client

from antirobot.scripts.log_viewer.config import Config
from antirobot.scripts.log_viewer.app import precalc_mgr
from antirobot.scripts.log_viewer.app import log_mgrs
from antirobot.scripts.log_viewer.app import calc_redirects

from antirobot.scripts.utils import log_setup


def ParseArgs():
    parser = argparse.ArgumentParser()

    parser.add_argument('--key', action='store', help="key type (ip, yuid or all)")
    parser.add_argument('--log-id', action='store', dest='logId', help="log id (%s or all)" % ','.join(log_mgrs.GetLogIds()))
    parser.add_argument('--date', action='store', help='date to handle in form of YYYMMDD')
    parser.add_argument('--max-days', dest='maxDays', action='store', type=int, help='number of days to precalc (default maximum possible)')
    parser.add_argument('--calc-redirects', dest='calc_redirects', action='store_true', help="Calculate redirects")
    parser.add_argument('--fast-logs', dest='fastLogs', action='store_true', help="Precalc fast logs")
    parser.add_argument('-q', dest='quiet', action='store_true', help='Do not print extra messages')
    parser.add_argument('cmd', help="command ('run' is only choice)", nargs='?')

    args = parser.parse_args()
    if args.cmd != 'run':
        parser.print_help()

        sys.exit(2)
    return args


def StrToDate(s):
    return datetime.datetime.strptime(s, '%Y%m%d').date()


def DoPrecalc(ytInst, clusterRoot, date, mgrList, maxDays, fastLogs):
    def ForEach(func):
        for mgrCls in mgrList:
            mgr = mgrCls(ytInst, clusterRoot)
            func(mgr)

    if fastLogs:
        ForEach(lambda mgr: mgr.PrecalcFastLogs())
        return

    if date:
        ForEach(lambda mgr: mgr.PrecalcOne(StrToDate(date)))
        return

    ForEach(lambda mgr: mgr.RunDayly(maxDays=maxDays))


def Precalc(ytInst, clusterRoot, logId=None, key=None, date = None, maxDays = None, fastLogs=False):
    if not maxDays:
        maxDays = precalc_mgr.MAX_DATE_BACK

    mgrList = log_mgrs.GetMgrList(logId, key)

    if not mgrList:
        print >>sys.stderr, 'Empty list of modules - exiting.'
        sys.exit(2)

    yt_client.MkDirSafe(ytInst, clusterRoot)
    DoPrecalc(ytInst, clusterRoot, date, mgrList, maxDays, fastLogs)


def CalcRedirectsIfNeed(ytInst, endDate, clusterRoot, ytProxy, ytToken, quiet=False):
    SUNDAY = 6

    if endDate.weekday() != SUNDAY:
        return

    ytInst = yt_client.GetYtClient(ytProxy, ytToken)
    lockName = os.path.join(clusterRoot, 'calc_redirects.lock')
    yt_client.MkDirSafe(ytInst, lockName)
    with ytInst.Transaction() as tr:
        try:
            ytInst.lock(lockName)
        except Exception, ex:
            pass
        else:
            calc_redirects.CalcRedirectsCount(ytInst, clusterRoot, endDate, dayCount=7, verbose=not quiet)


def main():
    args = ParseArgs()

    log_name = 'precalc_fast.log' if args.fastLogs else 'precalc.log'
    log_setup.Setup(os.path.join(Config.LOG_DIR, log_name), level=logging.INFO)
    logging.getLogger("Yt").setLevel(logging.ERROR)
    logging.info('=== precalc.py started ===')

    try:
        yt.config['token'] = Config.YT_TOKEN
        yt.config['proxy']['url'] = Config.YT_PROXY

        ytInst = yt_client.GetYtClient(Config.YT_PROXY, Config.YT_TOKEN, loggerName=__name__)
        if args.logId and args.key:
            Precalc(ytInst, Config.CLUSTER_ROOT, logId=args.logId, key=args.key, date=args.date, maxDays = args.maxDays, fastLogs=args.fastLogs)

        if args.calc_redirects:
            endDate = datetime.date.today() - datetime.timedelta(days=1)
            if args.date:
                endDate = StrToDate(args.date)
            CalcRedirectsIfNeed(ytInst, endDate, Config.CLUSTER_ROOT, Config.YT_PROXY, Config.YT_TOKEN, quiet=args.quiet)

    except Exception, ex:
        logging.exception(ex)
        raise


if __name__ == "__main__":
    main()
