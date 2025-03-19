#!/usr/bin/env python3

import datetime
import re
import requests
import subprocess
import sys

def main():
    try:
        res = requests.get('http://localhost:8080/v1/status/')
    except:
        try:
            subprocess.check_call('zk get nocdev-zk-flock/irrd'.split(), stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            print("PASSIVE-CHECK:irrd-alive;0;Daemon is down, but lock is active")
        except:
            print("PASSIVE-CHECK:irrd-alive;2;Failed to get status")
        sys.exit(0)
    else:
        print("PASSIVE-CHECK:irrd-alive;0;OK")

    sources = dict()
    for ln in res.text.splitlines():
        if re.match('^Status for ', ln):
            src = ln.split()[-1]
            sources[src] = dict()

        if re.match('\s{4}Last update:', ln):
            last_update = ':'.join(ln.strip().split(':')[1:])
            last_update = re.sub(':(\d{2})$', lambda m: m.group(0)[1::], last_update)
            last_update = datetime.datetime.strptime(last_update.strip(), '%Y-%m-%d %H:%M:%S.%f%z').timestamp()
            sources[src]['last_update'] = last_update

        if re.match('\s{4}Newest serial number mirrored:',ln):
            sn_mirrored = ln.split(':')[1].strip()
            if sn_mirrored.isdigit():
                sources[src]['sn_mirrored'] = int(sn_mirrored)
            elif sn_mirrored == 'None':
                sources[src]['sn_mirrored'] = None

        if re.match('\s{4}Newest journal serial number:',ln):
            sn_newest = ln.split(':')[1].strip()
            if sn_mirrored.isdigit():
                sources[src]['sn_newest'] = int(sn_newest)
            elif sn_mirrored == 'None':
                sources[src]['sn_newest'] = None

    status = 0
    msg = ''
    now = datetime.datetime.now().timestamp()
    for src, values in sources.items():
        if 'sn_newest' in values and 'sn_mirrored' in values and values['sn_newest'] == values['sn_mirrored']:
            continue

        lag = now - values['last_update']
        if lag > 3600*24:
            status = 2
            msg += 'CRIT: %s is outdated for %d+ hours\n' % (src, lag/3600)
        elif lag > 3600*2:
            status = max(status, 1)
            msg += 'WARN: %s is outdated for %d+ hours\n' % (src, lag/3600)

    print("PASSIVE-CHECK:irrd-sync;%d;%s" % (status, msg or 'OK'))

if __name__ == '__main__':
  main()
