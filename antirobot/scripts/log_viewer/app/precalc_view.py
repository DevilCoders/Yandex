import os
import subprocess
import time

from flask import request, url_for, render_template, redirect

import misc
import log_mgrs
import precalc_mgr
import yt_client

from . import app


def CheckDate(dateStr):
    time.strptime(dateStr, "%Y%m%d")


class PrecalcRunner:
    proc = None
    precalcScript = os.path.join(os.path.dirname(os.path.realpath(__file__)), 'precalc.py')
    curLogId = None
    curKey = None
    curDate = None

    @classmethod
    def IsRunning(cls):
        if not cls.proc:
            return None

        return cls.proc.poll() == None

    @classmethod
    def Start(cls, confPath, logId='all', key='all', date=None, maxDays=None):
        if cls.IsRunning():
            return False

        cls.curLogId = logId
        cls.curKey = key
        args = [cls.precalcScript, '--log-id', logId, '--key', key, '-c', confPath]
        if date:
            CheckDate(date)
            args.extend(['--date', str(date)])
            cls.curDate = date

        if maxDays:
            args.extend(['--max-days', str(maxDays)])

        args.append('run')
        print args
        cls.cmdLine = ' '.join(args)
        cls.proc = subprocess.Popen(args, close_fds=True)

        return cls.IsRunning()

    @classmethod
    def CurPrecalcTable(cls):
        res = None
        if cls.IsRunning():
            res = "%s by '%s' key" % (cls.curLogId, cls.curKey)
            if cls.curDate:
                res += ' on %s' % cls.curDate

        return res


@app.route('/viewprecalced', methods=['GET', 'POST'])
def ViewPrecalcedTables():
    conf = app.config['MY_CONFIG']
    validationError = None
    result = request.args.get('result', None)

    precalcProc = PrecalcRunner()

    if request.method == 'POST':
        if precalcProc.IsRunning():
            return redirect(url_for('ViewPrecalcedTables'))

        if request.form['kind'] == 'all':
            res = precalcProc.Start(conf.ConfPath)
            return redirect(url_for('ViewPrecalcedTables', result=int(res)))
        else:
            date = request.form['date']
            logId = request.form['log_id']
            key = request.form['key']

            if date and key:
                res = precalcProc.Start(conf.ConfPath, logId=logId, key=key, date=date, maxDays=1)
                return redirect(url_for('ViewPrecalcedTables', result=int(res)))

            validationError = 'You must specify date and table type'

    ytInst = yt_client.GetYtClient(conf.YT_PROXY, conf.YT_TOKEN)

    allMgrs = log_mgrs.GetMgrList()

    tables = []
    for mgrCls in allMgrs:
        mgr = mgrCls(ytInst, conf.CLUSTER_ROOT)
        tables += mgr.MissedTables(precalc_mgr.MAX_DATE_BACK)

    logNames = log_mgrs.GetLogIds()

    keyNames = (
        ('all', 'all'),
        ('IP', precalc_mgr.IP_FIELD),
        ('YandexUid', precalc_mgr.YUID_FIELD)
        )

    return render_template('precalc_view.html',
                curHref='precalc_view.py',
                tables=tables,
                inProcess=precalcProc.IsRunning(),
                startResult=result,
                nowProcessing=precalcProc.CurPrecalcTable(),
                logNames=logNames,
                keyNames=keyNames
            )

