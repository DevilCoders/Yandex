#!/usr/bin/python

import sys
import os
import socket
import re
import subprocess
import threading
import json
import random


ping_file = '/var/tmp/pre_ping'

if len(sys.argv) > 1:
    ipv6 = True
else:
    ipv6 = False

if os.path.isfile(ping_file):
    f = open(ping_file)
    DCS = json.load(f)
    f.close()
else:
    DCS = {}

CONDUCTOR_RE = re.compile("^\s*(\S+)\s+(\S+)\s*$")
PING_RE = re.compile("rtt min/avg/max/mdev\s+=\s+(.*?)/(.*?)/(.*?)/(.*?)\s+ms")
LOSS_RE = re.compile(".* (\d+)% packet loss")


class PingTester:

    def __init__(self):
        self.hostname = socket.gethostname()
        self.dcs = DCS

    def ping(self, hostname):
        time = '0.2'
        if os.getuid() != 0:
            time = '0.4'

        ping_v = '/bin/ping'
        if ipv6:
            ping_v = '/bin/ping6'

        p = subprocess.Popen([ping_v, "-q", "-c", "12", "-W", "2", "-i",
                              time, hostname], stderr=subprocess.PIPE, stdout=subprocess.PIPE)
        loss = '0'
        for line in p.stdout:
            m = re.match(PING_RE, line)
            match_loss = LOSS_RE.findall(line)
            if len(match_loss):
                loss = match_loss[0]
            if m is not None:
                return "%s:%s:%s" % (m.group(2), loss, hostname)
        return "%s:%s:%s" % ("0", loss, hostname)

    def pingThreadWrapper(self, hostname, pipe):
        try:
            ping = self.ping(hostname)
        except:
            ping = "0"
        pipe.send(ping)
        pipe.close()
        return 0

    def formatResult(self, my_dc, result):
        res = "my_dc:%s " % my_dc
        i = 0
        for dc, ping in result.iteritems():
            if i > 0:
                res += " "
            res = res + "%s:%s" % (dc, ping)
            i += 1
        return res

    def test(self):
        result = {}
        socks = []
        thr = []
        my_dc = self.dcs['my_dc']
        for dc, hosts in self.dcs.iteritems():
            if dc == 'my_dc':
                continue
            s1, s2 = socket.socketpair(socket.AF_UNIX, socket.SOCK_STREAM)
            socks.append((s1, dc))
            if len(hosts) > 1 and dc == my_dc:
                tmp_hosts = hosts
                try:
                    tmp_hosts.pop(tmp_hosts.index(self.hostname))
                except:
                    pass
                host = tmp_hosts[random.randint(0, len(hosts) - 1)]
            else:
                host = hosts[random.randint(0, len(hosts) - 1)]

            t = threading.Thread(
                target=self.pingThreadWrapper, args=(host, s2))
            thr.append(t)
            t.start()
        for t in thr:
            t.join(5)
        for s, dc in socks:
            repl = None
            while not repl:
                repl = s.recv(64)
            result[dc] = repl
            s.close()
        return self.formatResult(my_dc, result)


if __name__ == "__main__":
    try:
        p = PingTester()
    except:
        print 'ERROR'
        sys.exit(0)

    print p.test()
