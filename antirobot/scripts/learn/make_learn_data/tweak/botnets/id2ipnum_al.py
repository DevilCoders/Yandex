#!/usr/bin/env python

from datetime import date, timedelta
from operator import attrgetter
from itertools import imap
from random import randrange

from count_max_ips import count_max_ips, IpCounter
from id2time_ip import id2time_ip

import yt.wrapper as yt

DEF_THREAD_NUM = 4

class RightShifter(object):
    def __init__(self, bits_num):
        self.bits_num = bits_num
    def __call__(self, ip):
        return ip >> self.bits_num

subnet24 = RightShifter(8)
subnet16 = RightShifter(16)

secs_per_hour = 60 * 60
secs_per_day = secs_per_hour * 24

IpCounters = map(IpCounter, *zip(
        ('day',     None,       secs_per_day,   10),
        ('hour',    None,       secs_per_hour,  10),
        ('day24',   subnet24,   secs_per_day,   10),
        ('hour24',  subnet24,   secs_per_hour,  10),
        ('day16',   subnet16,   secs_per_day,   10),
        ('hour16',  subnet16,   secs_per_hour,  10),
) )

Cookies = 'spravka', 'fuid01', 'L'


#def RunMap(tmpDir, dateStart, dateEnd):
#    max_window = max(imap(attrgetter('time_window'), IpCounters))
#    moreDays = (max_window + secs_per_day - 1) // secs_per_day
#    dateBegin -= timedelta(moreDays)
#    dateEnd += timedelta(moreDays)
#    id2time_ip('access_log/', tmpDir, cookies, dateStart, dateEnd, DEF_THREAD_NUM)

#def RunReduce(tmpDir, dstPrefix):
#
#    count_max_ips(tmp_dir, dst_prefix, cookies, ip_counters, thread_num)

def id2ipnum_al(src_prefix='access_log/',
                dst_prefix='antirobot/ipnum/',
                cookies=Cookies,
                ip_counters=IpCounters,
                date_begin=date.today() - timedelta(date.today().day - 1),
                date_end=date.today(),
                thread_num=10):
    max_window = max(imap(attrgetter('time_window'), IpCounters))
    more_days = (max_window + secs_per_day - 1) // secs_per_day
    date_begin -= timedelta(more_days)
    date_end += timedelta(more_days)
    tmp_dir = 'tmp/antirobot_%08x/' % randrange(0xffffffff)

    id2time_ip(src_prefix, tmp_dir, cookies, date_begin, date_end, thread_num)
    count_max_ips(tmp_dir, dst_prefix, cookies, ip_counters, thread_num)

    for c in cookies:
        MapReduce.dropTable(tmp_dir + c)


if __name__ == '__main__':
    MapReduce.useDefaults(verbose=True, usingSubkey=False, workDir='.')
    id2ipnum_al()
