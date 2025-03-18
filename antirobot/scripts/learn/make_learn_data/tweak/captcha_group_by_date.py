#!/usr/bin/env python

import os
import sys
import datetime

THIS_DIR = os.path.dirname( os.path.realpath(__file__))
sys.path.append(THIS_DIR + '/..')
sys.path.append(THIS_DIR + '/../..')
sys.path.append(THIS_DIR + '../../antirobot_eventlog/common')
sys.path.append(THIS_DIR + '/../../utils')

from mapreducelib import MapReduce, Record, TemporaryTable


def Map(rec):
    fs = rec.value.split()
    date = datetime.date.fromtimestamp(int(fs[0]) / 1000000)
    key = '%s:%s' % (date.strftime('%Y%m%d'), fs[3])

    yield Record(key, '', rec.value)


def Reduce(key, records):
    (date, ip) = key.split(':')
#    if len(list) < 10:
    totalSessions = 0
    successSessions = 0
    attempts = 0
    for r in records:
        totalSessions += 1
        fs = r.value.split()
        attempts += int(fs[12])
        if int(fs[5]) == 1:
            successSessions += 1
#yield Record(fs[0], '', r.value)
    yield Record(date, ip, '\t'.join((str(totalSessions), str(successSessions), str(attempts))))

#def ReduceRes(key, records):
#    count = 0
#    for r in records:
#        count += int(r.value)
#
#    yield Record(key, str(count))

def main():
    MapReduce.useDefaults(lenvalMode=True, usingSubkey = True, appendMode = False, verbose=True, testMode=False)
#    for i in sys.stdin:
#        fs = i.split()
#        date = datetime.date.fromtimestamp(int(fs[0]) / 1000000)

#        print '\t'.join(['%s_%s' % (date.strftime('%Y%m%d'), fs[3])] + fs)
#    cnt = map.get(date, 0)
#    cnt += 1
#    map[date] = cnt





    MapReduce.runMap(Map, srcTable='tmp/dude/tot1', dstTable='tmp/dude/tot_map')
    MapReduce.runReduce(Reduce, srcTable='tmp/dude/tot_map', dstTable='tmp/dude/tot_red')
#    MapReduce.runReduce(ReduceRes, srcTable='tmp/dude/tot_red', dstTable='tmp/dude/tot_res')

if __name__ == '__main__':
    main()
