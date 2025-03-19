#!/usr/bin/env python3
"""
Checks AppArmor's blocks
"""

import json
import subprocess
import re
import os
import sys


STATE_WARN = '/var/log/mdb-apparmor-check-warn.log'
STATE_CRIT = '/var/log/mdb-apparmor-check-crit.log'
INTERVAL = '600'


def die(status=0, message='OK'):
    """
    Print status in monrun-compatible format
    """
    print('{status};{message}'.format(status=status, message=message))
    sys.exit(0)


def parse_logs(state_warn=STATE_WARN, state_crit=STATE_CRIT):
    """
    Parse apparmor's logs and write it in state_file
    """
    logs = subprocess.Popen(['sudo', 'journalctl', '--since', '"{}sec ago"'.format(INTERVAL)],
                            stdout=subprocess.PIPE, stderr=subprocess.DEVNULL)
    while True:
        line = logs.stdout.readline()
        if not line:
            break
        line_str = line.decode("utf-8")
        if 'apparmor="DENIED"' in line_str:
            # Get AppArmor profiles
            raw = subprocess.Popen(['sudo', 'aa-status', '--json'],
                                    stdout=subprocess.PIPE, stderr=subprocess.DEVNULL)
            profiles = json.loads(raw.stdout.read())['profiles']
            profile = re.search('profile="(.+?)"', line_str).group(1)
            if profile in profiles and profiles[profile] == 'enforce':
                with open(state_crit, 'a') as f:
                    f.write(line_str)
                    continue
            with open(state_warn, 'a') as f:
                f.write(line_str)


def main():
    parse_logs()
    error_code = 0
    message = 'OK'
    if os.path.isfile(STATE_WARN) and os.path.getsize(STATE_WARN) > 0:
        error_code = 1
        message = 'Denied events were found for complained profiles'
    if os.path.isfile(STATE_CRIT) and os.path.getsize(STATE_CRIT) > 0:
        error_code = 2
        message = 'Denied events were found for enforced profiles'
    die(error_code, message)


if __name__ == '__main__':
    main()
