#!/skynet/python/bin/python

import sys
from optparse import OptionParser
import urllib
import time
import re
import socket as socket_origin

from gevent import monkey, socket, with_timeout
from gevent.pool import Pool


ADDR_FAMILY = 0
ADDR_ADDR = 4

def CheckArgs(opts, args):
    if len(args) < 2:
        print >>sys.stderr, "Pls specify host and port"
        return False

    if opts.delayed and opts.useTs:
        print >>sys.stderr, "Erorr: options '--delayed' and '--use-ts' are mutually exclusive"
        return False

    return True


def ParseArgs():
    parser = OptionParser(\
'''Usage: %prog [options] <host> <port>
This script sends requests to some host and port.
'''
)
    parser.add_option('-s', '', dest='maxThreads', action='store', type='int', default = 1, help='simultaneous outstanding requests. default = 1')
    parser.add_option('-v', '--verbose', dest='verbose', action='store_true', default = False, help='be verbose')
    parser.add_option('-r', '--repeat', dest='repeat', action='store', type='int', default = 1, help='number of times to repeat each request')
    parser.add_option('', '--delayed', dest='delayed', action='store', type='int', default = 0, help='delay in milliseconds between bytes on request sending')
    parser.add_option('', '--batch', dest='batch', action='store_true', default = False, help='read urlencoded requests from stdin in form of <timestamp>\\t<request>')
    parser.add_option('', '--no-fr', dest='noFr', action='store_true', default = False, help='do not wrap requests into /fullreq')
    parser.add_option('', '--use-ts', dest='useTs', action='store_true', default = False, help='if --batch is in effect, send the requests according to timestamps')
    parser.add_option('', '--tweak-ts', dest='tweakTs', action='store_true', default = False, help='if --use-ts is in effect, update request timestamp to current value')
    parser.add_option('', '--progress', dest='progress', action='store_true', default = False, help='show progress')
    parser.add_option('', '--stat', dest='showStat', action='store_true', default = False, help='show statistics')
    parser.add_option('', '--show-errors', dest='showErrors', action='store_true', default=False, help='show error on sockets')
    parser.add_option('', '--stop-on-error', dest='stopOnError', action='store_true', default=False, help='show error on sockets')
    parser.add_option('', '--timeout', dest='timeout', action='store', type='int', default=None, help='set timeout for connect and read in milliseconds')

    (opts, args) = parser.parse_args()

    if not CheckArgs(opts, args):
        sys.exit(2)

    return (opts, args);


reTimeStamp = re.compile(r'X-Start-Time: \S+', re.MULTILINE)

def UpdateReqTimeStamp(req):
    return reTimeStamp.sub('X-Start-Time: %d' % int(time.time() * 1000000), req)

def SingleReq(inp):
    yield inp.read()

class  BatchGetReqs:
    def __init__(self, useTimeDeltas, tweakTimestamp=False):
        self.prevReqTS = None
        self.useTimeDeltas = useTimeDeltas
        self.lastPrintDiff = time.time()
        self.tweakTimestamp = tweakTimestamp

    @staticmethod
    def Parse(line):
        fs = line.split('\t', 1)
        if len(fs) == 2:
            return (int(fs[0]) / 1000000, urllib.unquote_plus(fs[1].rstrip()))
        else:
            return (None, urllib.unquote_plus(fs[0].rstrip()))

    def Wait(self, curReqTS):
        if curReqTS is None:
            print >>sys.stderr, "Bad timestamp value"
            sys.exit(1)

        now = time.time()
        nextInStream = curReqTS + self.delta
        if now < nextInStream:
            sleepTime = nextInStream - now
            time.sleep(sleepTime)
        else:
            if now - self.lastPrintDiff > 10:
                print >>sys.stderr, "Time diff = %f" % (now - nextInStream)
                self.lastPrintDiff = now

        self.prevReqTS = curReqTS

    def __call__(self, inp):
        (self.prevReqTS, req) = self.Parse(inp.next())
        if self.prevReqTS != None:
            self.delta = time.time() - self.prevReqTS

        yield UpdateReqTimeStamp(req) if self.tweakTimestamp else req

        for i in inp:
            (timestamp, req) = self.Parse(i)
            if self.useTimeDeltas:
                self.Wait(timestamp)

            yield UpdateReqTimeStamp(req) if self.tweakTimestamp else req


class TimeMeasure:
    def __init__(self):
        self.Reset()

    def Reset(self):
        self.start = time.time()
        self.curtime = self.start

    def Step(self):
        cur = time.time()
        res = cur - self.curtime
        self.curtime = cur
        return res

    def Total(self):
        return time.time() - self.start


def DelayedSendReq(sock, fullReq, delay, packetSize = 32):
    if delay == 0:
        sock.sendall(fullReq)
        return

    sent = 0
    total = len(fullReq)
    toSleep = float(delay) / 1000
    while sent < total:
        toSend = min(packetSize, total - sent)
        sock.sendall(fullReq[sent:sent + toSend])
        sent += toSend
        time.sleep(toSleep);


def Verbose(msg):
    print msg

class TimeAggr:
    def __init__(self):
        self.sum = 0
        self.min = None
        self.max = None

    def Update(self, value):
        if not value is None:
            self.sum += value
        if self.min is None or value < self.min:
            self.min = value
        if self.max is None or value > self.max:
            self.max = value


class Stat:
    def __init__(self):
        self.sentTotal = 0
        self.connectSuccess = 0
        self.sentSuccess = 0
        self.getSuccess = 0
        self.sessionSuccess = 0

        self.errorsTotal = 0
        self.errorsClosedUnexpect = 0
        self.errorsSocket = 0

        self.connectTimeouts = 0
        self.readTimeouts = 0

        self.timeConnect = TimeAggr()
        self.timeSendReq = TimeAggr()
        self.timeGetAnswer = TimeAggr()
        self.timeSession = TimeAggr()


    def IncSentTotal(self):
        self.sentTotal += 1


    def IncConnectSuccess(self):
        self.connectSuccess += 1


    def IncSentSuccess(self):
        self.sentSuccess += 1


    def IncGetSuccess(self):
        self.getSuccess += 1


    def IncSessionSuccess(self):
        self.sessionSuccess += 1


    def IncConnectTimeouts(self):
        self.connectTimeouts += 1


    def IncReadTimeouts(self):
        self.readTimeouts += 1


    def IncErrorClosedUnexpect(self):
        self.errorsTotal += 1
        self.errorsClosedUnexpect += 1


    def IncErrorSocket(self):
        self.errorsTotal += 1
        self.errorsSocket += 1


    def Print(self):
        values = self.__dict__

        Fix = lambda v: '%.6fs' % v if v else 'N/A'

        values['timeConnectMin'] = Fix(self.timeConnect.min)
        values['timeConnectMax'] = Fix(self.timeConnect.max)
        values['timeSendReqMin'] = Fix(self.timeSendReq.min)
        values['timeSendReqMax'] = Fix(self.timeSendReq.max)
        values['timeGetAnswerMin'] = Fix(self.timeGetAnswer.min)
        values['timeGetAnswerMax'] = Fix(self.timeGetAnswer.max)
        values['timeSessionMin'] = Fix(self.timeSession.min)
        values['timeSessionMax'] = Fix(self.timeSession.max)

        values['totalTimeouts'] = self.connectTimeouts + self.readTimeouts
        values['timeConnectAvg'] = '%.6fs' % (self.timeConnect.sum / self.connectSuccess) if self.connectSuccess  else 'N/A'
        values['timeSendReqAvg'] = '%.6fs' % (self.timeSendReq.sum / self.sentSuccess) if self.sentSuccess > 0 else 'N/A'
        values['timeGetAnswerAvg'] = '%.6fs' % (self.timeGetAnswer.sum / self.getSuccess) if self.getSuccess > 0 else 'N/A'
        values['timeSessionAvg'] = '%.6fs' % (self.timeSession.sum / self.sessionSuccess) if self.sessionSuccess > 0 else 'N/A'

        print """
Total requests sent: %(sentTotal)d
Success connect: %(connectSuccess)d
Success sent: %(sentSuccess)d
Success answer get: %(getSuccess)d

Total errors: %(errorsTotal)d
    closed unexpectedly: %(errorsClosedUnexpect)d
    socket errors: %(errorsSocket)d

Timeouts: %(totalTimeouts)d
    connect: %(connectTimeouts)d
    read: %(readTimeouts)d

Connect timings:
    min: %(timeConnectMin)s
    max: %(timeConnectMax)s
    avg: %(timeConnectAvg)s

Sent timings:
    min: %(timeSendReqMin)s
    max: %(timeSendReqMax)s
    avg: %(timeSendReqAvg)s

Answer get timings:
    min: %(timeGetAnswerMin)s
    max: %(timeGetAnswerMax)s
    avg: %(timeGetAnswerAvg)s

Whole session timings:
    min: %(timeSessionMin)s
    max: %(timeSessionMax)s
    avg: %(timeSessionAvg)s
""" % values


stop = False

class Session:
    class ConnectTimeout(Exception):
        pass

    class ReadTimeout(Exception):
        pass

    def __init__(self, opts, stat):
        self.measure = TimeMeasure()
        self.timeConnect = None
        self.timeSendRequest = None
        self.timeGetAnswer = None
        self.timeTotal = None
        self.opts = opts
        self.stat = stat
        stat.IncSentTotal()

    def __call__(self, addrInfo, req):
        global stop

        timeout = float(self.opts.timeout) / 1000 if self.opts.timeout else sys.maxint
        try:
            sock = socket.socket(addrInfo[ADDR_FAMILY])

            self.measure.Reset()
            res = with_timeout(timeout, sock.connect, addrInfo[ADDR_ADDR], timeout_value = True)
            if res == True:
                #self.stat.IncConnectTimeouts()
                raise self.ConnectTimeout

            self.timeConnect = self.measure.Step()
            self.stat.IncConnectSuccess()
            if stop:
                return

            Verbose("* sending request\n%s" % req)
            self.measure.Step()
            DelayedSendReq(sock, req, self.opts.delayed)
            self.timeSendRequest = self.measure.Step()
            self.stat.IncSentSuccess()
            if stop:
                return

            Verbose("* waiting for answer...")
            self.measure.Step()
            ret = with_timeout(timeout, sock.recv, 8192, timeout_value = True)
            if ret == True:
                #self.stat.IncReadTimeouts()
                #return
                raise self.ReadTimeout

            self.timeGetAnswer = self.measure.Step()
            self.timeTotal = self.measure.Total()
            Verbose('* answer got, size %d bytes\n%s' % (len(ret), ret))
            if stop:
                return

            if not ret:
                if self.opts.showErrors:
                    print >>sys.stderr, "Connection closed unexpectedly"

                if self.opts.stopOnError:
                    stop = True

                self.stat.IncErrorClosedUnexpect()

            self.stat.IncGetSuccess()
            self.stat.IncSessionSuccess()

            sock.close()

        except KeyboardInterrupt:
            stop = True

        except self.ConnectTimeout:
            self.stat.IncConnectTimeouts()

        except self.ReadTimeout:
            self.stat.IncReadTimeouts()

        except Exception, ex:
            if self.opts.stopOnError:
                stop = True
            self.stat.IncErrorSocket()
            if self.opts.showErrors:
                print >>sys.stderr, str(ex)

        self.stat.timeConnect.Update(self.timeConnect)
        self.stat.timeSendReq.Update(self.timeSendRequest)
        self.stat.timeGetAnswer.Update(self.timeGetAnswer)
        self.stat.timeSession.Update(self.timeTotal)


def GetAddrInfo(host, port):
    try:
        return socket_origin.getaddrinfo(host, port)[0]
    except:
        print >>sys.stderr, "Could not resolve address pair %s:%s" % (host, port)
        sys.exit(1)


def MakeFullReq(req):
    length = len(req)

    return \
'''POST /fullreq HTTP/1.1\r
Content-Length: %d\r
\r
%s''' % (length, req)

def main():
    (opts, args) = ParseArgs()

    host = args[0]
    port = args[1]

    addrInfo = GetAddrInfo(host, port)
    monkey.patch_all()

    if not opts.verbose:
        global Verbose
        Verbose =  lambda x: None

    stat = Stat()

    pool = Pool(opts.maxThreads)

    repeat = opts.repeat
    GetReqs = SingleReq
    if opts.batch:
        GetReqs = BatchGetReqs(opts.useTs, opts.tweakTs)

    step = 1000 if opts.repeat >= 10000 else 100

    WrapReq = MakeFullReq if not opts.noFr else lambda r: r

    if not opts.batch and opts.tweakTs:
        prevWrap = WrapReq
        WrapReq = lambda x: prevWrap(UpdateReqTimeStamp(x))

    try:
        for req in GetReqs(sys.stdin):
            if stop:
                break

            for j in xrange(repeat):
                if stop:
                    break

                pool.spawn(Session(opts, stat), addrInfo, WrapReq(req))

                if opts.progress:
                    if stat.sentTotal > 1000:
                        step == 1000

                    if stat.sentTotal % step == 0:
                        print "%d (errors: %d)" % (stat.sentTotal, stat.errorsTotal)

        pool.join()
    except KeyboardInterrupt:
        pool.join()

    if opts.showStat:
        stat.Print()

if __name__ == "__main__":
    main()
