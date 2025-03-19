#!/usr/bin/python -u
# -*- coding: utf-8 -*-

import sys
import os
import socket
import shutil
import json
import requests
import time


'''
Endrule examples:
>> [0:0] -A Y_PVOk:0:41b9:eb7b:8aac_OUT -s 2a02:6b8:fc00::/40 -m u32 --u32 "0x10&0xffffffff=0x41b9" -j Y18_FROM_41b9@ya:fc00::/40
>> [1096080264:3592950816688] -A Y_gWkp:0:41b9:eb7b:8aac_OUT -s 2a02:6b8:c00::/40 -m u32 --u32 "0x10&0xffffffff=0x41b9" -j Y5_FROM_41b9@ya:c00::/40

Hbfrule examples:
*filter
:NOC_ALLOW_INPUT -
-A INPUT -p tcp --dport 22 -j SSH_EXCEPTION_INPUT
-A NOC_ALLOW_INPUT -s 5.45.203.96/27 -j ACCEPT
'''



RT_MACROS = ['_NOCRTSRV_', '_NOCSRVNETS_']
RESOLVURI = 'http://hbf.yandex.net/macros/{0}'
DEFAULT_SET = []
REQ_TIMEOUT = 5
TMPRULEFILE4 = '/var/tmp/10-noc-allow.v4'
TMPRULEFILE6 = '/var/tmp/10-noc-allow.v6'
RULEFILE4DST = '/etc/yandex-hbf-agent/rules.d/10-noc-allow.v4'
RULEFILE6DST = '/etc/yandex-hbf-agent/rules.d/10-noc-allow.v6'
RULEFILEHEAD = '*filter\n:NOC_ALLOW_INPUT -\n-A INPUT -p tcp -m multiport --dports 80,443 -j NOC_ALLOW_INPUT\n'
TIMETOALARM = 604800
LINETOALARM = 3


# 1st we check that rule is exist, 2nd that rule count enought lines, 3th that rule not too old.
def monrun_check(cmd):
    l4 = 0
    l6 = 0
    try:
        with open(RULEFILE4DST) as f:
            l4 = len(f.read().split('\n'))
        with open(RULEFILE6DST) as f:
            l6 = len(f.read().split('\n'))
    except Exception as e:
        print('2; CRIT, some file not fount: {} or {}.'.format(RULEFILE4DST, RULEFILE6DST))
        return
    if LINETOALARM >= l4 or LINETOALARM >= l6:
        print('2; CRIT, rule count too low, please check {} or {}.'.format(RULEFILE4DST, RULEFILE6DST))
        return
    ctime = int(time.time())
    fmtime4 = int(os.path.getmtime(RULEFILE4DST))
    fmtime6 = int(os.path.getmtime(RULEFILE6DST))
    if (ctime - fmtime4) > TIMETOALARM or (ctime - fmtime4) > TIMETOALARM:
        print('2; CRIT, rule too old, please check script {0} working.'.format(cmd))
        return
    print('0; OK')

def normalize_to_rule_root(net):
    answer4 = []
    answer6 = []
    if 'y' in net:
        addrts = socket.getaddrinfo(net, 80, 0, 0, socket.IPPROTO_TCP)
        for addrt in addrts:
            if addrt[0] == 10:
                answer6.append('{}/128'.format(addrt[4][0]))
            elif addrt[0] == 2:
                answer4.append('{}/32'.format(addrt[4][0]))
        return answer4, answer6
    elif '@' in net:
        pidnet = net.split('@')
        pid = pidnet[0]
        netprefix = pidnet[1]
        answer6.append('{0} -m u32 --u32 "0x10&0xffffffff=0x{1}"'.format(netprefix, pid))
    elif ':' in net:
        answer6.append(net)
    elif '.' in net:
        answer4.append(net)
    return answer4, answer6


def main(argv):

    if len(argv) > 1:
        if argv[1] == '--monrun':
            monrun_check(argv[0])
            return

    raw_nets = []
    for macros in RT_MACROS:
        req = requests.get(RESOLVURI.format(macros), timeout=REQ_TIMEOUT)
        raw_nets = raw_nets + req.json()

    root_rule4 = []
    root_rule6 = []
    for raw_net in raw_nets:
        a = normalize_to_rule_root(raw_net)
        root_rule4 = root_rule4 + a[0]
        root_rule6 = root_rule6 + a[1]

    if len(root_rule4):
        with open(TMPRULEFILE4, 'w') as f:
            f.write(RULEFILEHEAD)
            for rrule in root_rule4:
                f.write('-A NOC_ALLOW_INPUT -s {0} -j ACCEPT\n'.format(rrule))
        shutil.move(TMPRULEFILE4, RULEFILE4DST)

    if len(root_rule6):
        with open(TMPRULEFILE6, 'w') as f:
            f.write(RULEFILEHEAD)
            for rrule in root_rule6:
                f.write('-A NOC_ALLOW_INPUT -s {0} -j ACCEPT\n'.format(rrule))
        shutil.move(TMPRULEFILE6, RULEFILE6DST)


if (__name__ == "__main__"):
    sys.exit(main(sys.argv))

