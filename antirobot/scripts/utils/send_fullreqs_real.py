#!/usr/bin/env python

import sys;
import socket;
import time;
import datetime;
import re;
from optparse import OptionParser
import asyncore

DEBUG = False

def ParseArgs():
    parser = OptionParser(\
'''Usage: %prog [options] <host> <port> <rps>
Script sends fullreq requests to antirobot_daemon.
'''
)
    parser.add_option('-r', '--repeat', dest='repeat', action='store', type='int', default = 1, help='number of times to repeat each request')
    parser.add_option('', '--tweak-time', dest='tweakTime', action='store_true', default = False, help='tweak X-Start-Time header')
    parser.add_option('', '--debug', dest='debug', action='store_true', default = False, help='debug info')
    (opts, args) = parser.parse_args()

    if len(args) < 2:
        parser.print_usage(sys.stderr);
        sys.exit(2);

    global DEBUG
    DEBUG = opts.debug
    return (opts, args);

tweakTimeShift = None
timeRe = re.compile("^X-Start-Time: (.*?)$", re.MULTILINE);

# returns Req()
def readReq(f, tweakTime):
    global tweakTimeShift

    req = "";
    numConsEmptyLines = 0;
    timestamp = None
    while True:
        line = f.readline();
        if len(line) == 0:
            break;

        m = timeRe.search(line)
        if m:
            timestamp = m.group(1)
            dotPos = timestamp.find('.')
            if dotPos >= 0:
                timestamp = float(timestamp)
            else:
                timestamp = float(timestamp) / 1000000

            if tweakTime:
                if tweakTimeShift == None:
                    tweakTimeShift = time.time() - timestamp

                timestamp += tweakTimeShift
                line = 'X-Start-Time: %f\r\n' % timestamp

        if len(line.strip()) == 0:
            numConsEmptyLines += 1;
            if numConsEmptyLines == 2:
                break;
        else:
            numConsEmptyLines = 0;

        req += line;
    return (req, timestamp)

def now():
    return time.time();


class ARClient(asyncore.dispatcher):

    def __init__(self, addrInfo, fullReq):
        asyncore.dispatcher.__init__(self)
        self.create_socket(socket.AF_INET, socket.SOCK_STREAM)
        self.connect(addrInfo)
        self.buffer = fullReq

    def handle_connect(self):
        pass

    def handle_close(self):
        self.close()

    def handle_read(self):
        self.recv(8192)

    def writable(self):
        return (len(self.buffer) > 0)

    def handle_write(self):
        sent = self.send(self.buffer)
        self.buffer = self.buffer[sent:]

def MakeFullReq(req):
    return \
'''POST /fullreq HTTP/1.1\r
Content-Length: %d\r
\r
%s''' % (len(req), req)

def main():
    global rps;

    (opts, args) = ParseArgs()

    host = args[0];
    port = args[1];
    addrInfo = socket.getaddrinfo(host, port)[0][4];

    sockets = [];

    curTime = now();
    prevTime = now();
    numErrors = 0;

    prevTime = 0
    i = 0
    timeout = 10
    rpsT1 = now()
    rpsT2 = 0
    diffReal = 0
    (req, timestamp) = readReq(sys.stdin, opts.tweakTime)
    diffReal = now() - timestamp
    while True:
        i += 1

        wait = timestamp - prevTime if prevTime else 0
        prevTime = timestamp

        t1 = now();
        try:
            client = ARClient(addrInfo, MakeFullReq(req))
            asyncore.loop(timeout=wait > 0 if wait else 1, use_poll=True)
        except KeyboardInterrupt:
            raise;
        except:
            numErrors += 1;
            if numErrors > 10:
                raise;

        (req, timestamp) = readReq(sys.stdin, opts.tweakTime)
        if not req.strip():
            break;

        if not timestamp:
            timestamp = prevTime + 1
            print >>sys.stderr, "Empty timestamp on line %d" % i
            continue

        nowTime = now()
        diffSync = nowTime - (timestamp + diffReal)
        toSleep = -diffSync if diffSync < 0 else 0

        if toSleep > 20:
            print >>sys.stderr, "Line %d: I need to wait %fsec (timestamp = %f, now = %f, diffReal = %f)" % (i, toSleep, timestamp, nowTime, diffReal)

        if toSleep >= 0.0005:
#            print 'Waiting %fs' % toSleep
            time.sleep(toSleep);
#        else:
#            print 'No wait %fs' % toSleep
        if i % 1000  == 0:
            rpsT2 = now()
            diff = rpsT2 - rpsT1
            rpsT1 = rpsT2
            if diff > 0:
                print 'Rps: %f' % (float(1000) / diff),
            else:
                print 'Rps: too big',
            print 'timing diff: %f' % (now() - (timestamp + diffReal))



if __name__ == "__main__":
    main();

