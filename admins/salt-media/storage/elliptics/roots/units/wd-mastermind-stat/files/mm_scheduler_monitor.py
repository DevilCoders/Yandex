#!/usr/bin/pymds

import mastermind
import sys
import datetime
import logging

from mds.admin.library.python.sa_scripts import mm

logging.basicConfig(filename='/dev/null')

@mm.mm_retry(tries=5, delay=1, backoff=1.5)
def get_statistics():
    schuduler_client = mastermind.SchedulerClient(timeout=20)
    res = schuduler_client.scheduler_statistics(None)
    return res


try:
    res = get_statistics()

    if isinstance(res, dict) and 'Error' in res:
        print "1; {}".format(res)
        sys.exit()

    for starter, info in res['bunch_of_starters_infos'].iteritems():
        last_start_date = info['last_start_date']
        period = info['period']

        if last_start_date != 'never':
            time_delta = datetime.datetime.now() - datetime.datetime.strptime(last_start_date, "%Y-%m-%d %H:%M:%S")
            if time_delta.seconds > period * 5:
                print "2; Scheduler {} run {} sec ago".format(starter, time_delta.seconds)
                sys.exit()

    print "0; Ok"
except Exception as e:
    print "2; mastermind response error:", e
    exit()
