#!/usr/bin/env python

import sys;
import socket;
import time;
import datetime;
import re;
from optparse import OptionParser
from Cookie import SimpleCookie

DEBUG = False

def ParseArgs():
    parser = OptionParser(\
'''Usage: %prog [options] <host> <port> <rps>
Script sends fullreq requests to antirobot_daemon.
'''
)
    parser.add_option('-s', '--sockets', dest='maxSockets', action='store', type='int', default = 10, help='number of sockets')
    parser.add_option('-r', '--repeat', dest='repeat', action='store', type='int', default = 1, help='number of times to repeat each request')
    parser.add_option('', '--tweak-ip', dest='tweakIp', action='store_true', default = False, help='tweak X-Forwarded-For-Y header')
    parser.add_option('', '--set-ip', dest='setIp', action='store', default = None, help='modify X-Forwarded-For-Y header')
    parser.add_option('', '--set-fuid', dest='setFuid', action='store', default = None, help='add or modify fuid01 cookie')
    parser.add_option('', '--set-spravka', dest='setSpravka', action='store', default = None, help='add or modify spravka cookie')
    parser.add_option('', '--tweak-reqid', dest='tweakReqid', action='store_true', default = False, help='tweak X-Req-Id header')
    parser.add_option('', '--tweak-time', dest='tweakTime', action='store_true', default = False, help='tweak X-Start-Time header')
    parser.add_option('', '--current-time', dest='currentTime', action='store_true', default = False, help='use current time for tweak X-Start-Time header')
    parser.add_option('', '--debug', dest='debug', action='store_true', default = False, help='debug info')
    parser.add_option('', '--delayed', dest='delayed', action='store', type='int', default = 0, help='delay between bytes on request sending')
    parser.add_option('', '--damage', dest='damage', action='store_true', default = False, help='use invalid content-length when make fullreq')

    (opts, args) = parser.parse_args()

    if len(args) < 3:
        parser.print_usage(sys.stderr);
        sys.exit(2);

    global DEBUG
    DEBUG = opts.debug
    return (opts, args);


def readReq(f):
    req = "";
    numConsEmptyLines = 0;
    while True:
        line = f.readline();
        if len(line) == 0:
            break;

        if len(line.strip()) == 0:
            numConsEmptyLines += 1;
            if numConsEmptyLines == 2:
                break;
        else:
            numConsEmptyLines = 0;

        req += line;
    return req;

def now():
    return time.time();

curIp = 0;
tweakTime = 1300000000.0;
rps = 1.0;
xffyRe = re.compile("^X-Forwarded-For-Y: .*?$", re.MULTILINE);
timeRe = re.compile("^X-Start-Time: .*?$", re.MULTILINE);
reqidRe = re.compile("^X-Req-Id: .*?$", re.MULTILINE);
reCookies = re.compile("^Cookie: (.*?)$", re.MULTILINE)

def UpdateCookie(req, name, value):
    headers, data = req.split('\r\n\r\n')
    headers += '\r\n'

    cookStr = ''
    match = reCookies.search(headers)

    if match:
        cookStr = match.group(1)

    cook = SimpleCookie(cookStr)
    cook[name] = value

    newCookStr = 'Cookie:%s' % cook.output(header='', sep=';')

    #print "New cook: %s" % newCookStr
    if match:
        headers = reCookies.sub(newCookStr, headers)
    else:
        headers = '%s%s\r\n' % (headers, newCookStr)

    return '%s%s' % (headers, data)

def tweak(opts, fullReq):
    global curIp;
    global tweakTime;
    global rps;
    curIp += 1;
    tweakTime += 1.0 / rps;

    if opts.tweakIp:
        fullReq = xffyRe.sub("X-Forwarded-For-Y: %d.%d.%d.%d" % ((curIp >> 8) & 0xFF, curIp & 0xFF, (curIp >> 8) & 0xFF, curIp & 0xFF), fullReq);
        #fullReq = xffyRe.sub("X-Forwarded-For-Y: %d.%d.%d.%d" % ((curIp >> 24) & 0xFF, (curIp >> 16) & 0xFF, (curIp >> 8) & 0xFF, curIp & 0xFF), fullReq);
    elif opts.setIp:
        fullReq = xffyRe.sub("X-Forwarded-For-Y: %s" % opts.setIp, fullReq);

    if opts.tweakTime:
        fullReq = timeRe.sub("X-Start-Time: %d" % int(tweakTime * 1000000), fullReq);

    if opts.tweakReqid:
        fullReq = reqidRe.sub("X-Req-Id: %d" % curIp, fullReq);

    if opts.setFuid:
        fullReq = UpdateCookie(fullReq, 'fuid01', opts.setFuid)

    if opts.setSpravka:
        fullReq = UpdateCookie(fullReq, 'spravka', opts.setSpravka)

    return fullReq;

def MakeFullReq(req, damage):
    length = len(req)
    if damage:
        length += 1

    return \
'''POST /fullreq HTTP/1.1\r
Content-Length: %d\r
\r
%s''' % (length, req)

def DebugOut(msg):
    if DEBUG:
        print >>sys.stderr, msg,

def SendReq(sock, fullReq, delay, packetSize = 32):
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

def main():
    global rps;

    (opts, args) = ParseArgs()

    if opts.currentTime:
        global tweakTime
        tweakTime = time.time()
        print >>sys.stderr, "Using current time for time tweaking"

    host = args[0];
    port = args[1];
    rps = int(args[2]);
    addrInfo = socket.getaddrinfo(host, port)[0][4];

    DELAY = 1.0 / float(rps);
    print "delay: ", DELAY;

    i = 0;
    toSleep = 0.0;

    sockets = [];

    curTime = now();
    prevTime = now();
    numErrors = 0;
    while True:
        req = readReq(sys.stdin);
        if not req.strip():
            break;

        for ns in range(opts.repeat):
            fullReq = MakeFullReq(tweak(opts, req), opts.damage)
            DebugOut(fullReq + "\n")

            t1 = now();
            try:
                DebugOut("Creating socket...")
                s = socket.socket(socket.AF_INET6);
                DebugOut("Connecting...")
                s.connect(addrInfo);
                DebugOut("Connected.\nSending request...")
                SendReq(s, fullReq, opts.delayed)
                DebugOut("Getting answer...")
                ret = s.recv(4096);
                sockets.append(s);
                if not ret:
                    ret = '<EMPTY>'

                DebugOut("\nAnswer got: " + ret  + "\n")
            except KeyboardInterrupt:
                raise;
            except:
                numErrors += 1;
                if numErrors > 10:
                    raise;

            if len(sockets) >= opts.maxSockets:
                try:
                    sockets[0].recv(4096);
                except KeyboardInterrupt:
                    raise;
                except:
                    numErrors += 1;
                    if numErrors > 10:
                        raise;
                del sockets[0]

            t2 = now();
            if t2 - t1 < DELAY:
                toSleep += (DELAY - (t2 - t1));

            if toSleep >= 0.05:
                time.sleep(toSleep);
                toSleep = 0.0;

            i += 1;
            if i == 10000:
                curTime = now();
                print "%d reqs in %0.2f seconds, %0.2f rps" % (i, (curTime - prevTime), float(i) / (curTime - prevTime));
                i = 0;
                prevTime = curTime;

    while len(sockets) > 0:
        try:
            sockets[0].recv(4096);
        except:
            raise;
        del sockets[0]

if __name__ == "__main__":
    main();
