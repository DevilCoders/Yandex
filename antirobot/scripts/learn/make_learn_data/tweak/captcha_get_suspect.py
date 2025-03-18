#!/usr/bin/env python

import sys
import distr
import subprocess
import os

basePath = os.path.abspath(os.path.dirname(__file__))

DEBUG = bool(os.environ.get('DEBUG', False))

TIME_STEP = 1

def CalcTime(rec):
    return float(int(rec[11]) - int(rec[10])) / 1000000

class CaptchaResult:
    def __init__(self, fileName):
       self.fileName  = fileName

    def GetFullrecIterator(self):
        class Iter:
            def __init__(self, name):
                self.file = open(name)

            def __iter__(self):
                return self

            def next(self):
                line = self.file.readline()
                if line == '':
                    raise StopIteration
                return line.split()

        return Iter(self.fileName)

    def GetTimesIterator(self):
        for rec in self.GetFullrecIterator():
            if int(rec[5]) == 1:
                yield CalcTime(rec)

def TimeInPeaks(time, peaks):
    for p in peaks:
        if time >= p - TIME_STEP and time < p:
            return True
    return False

def main():
    if len(sys.argv) < 3:
        print "Usage %s dateStart dateEnd" % sys.argv[0]
        sys.exit(1)

#    captName = os.tmpnam()
#    f = open(captName, 'w')
#    p = subprocess.Popen([basePath + '/captcha_inputs.py',  sys.argv[1], sys.argv[2]], stdout=f)
#    p.wait()
#    f.close()


    if DEBUG: captName = 'test'

    cr = CaptchaResult(captName)

    dist = distr.Distribution(1, 100, TIME_STEP)

    dist.Iterate(cr.GetTimesIterator())

    distrResult = dist.GetResult()

    peaks = []
    prev = distrResult[0][0]
    for i in xrange(1, len(distrResult)-1):
        avr = (distrResult[i-1][1] + distrResult[i+1][1]) / 2
        if distrResult[i][1] > avr * 1.8:
            peaks.append(distrResult[i][0])
#            print '%s\t%d' % (distrResult[i][0], distrResult[i][1])

    ipCounts = {}
    for rec in cr.GetFullrecIterator():
        if int(rec[5]) == 1:
            time = CalcTime(rec)
#            print time
            if TimeInPeaks(time, peaks):
# print '\t'.join(rec)
                ip = rec[3]
                if ip in ipCounts:
                    ipCounts[ip] += 1
                else:
                    ipCounts[ip] = 1

    for ip, count in ipCounts.iteritems():
        print '\t'.join((ip, str(count)))


if __name__ =='__main__':
    main()
