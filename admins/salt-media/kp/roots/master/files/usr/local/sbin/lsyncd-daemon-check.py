#!/usr/bin/env python3
# MANAGED BY SALT

from subprocess import check_output, CalledProcessError
import sys
import os
import time
import psutil


THRESH_DAEMON_CTIME = 1800


def check_is_active_master():
    try:
        check_output('is-active-master')
        return True
    except CalledProcessError:
        return False


def check_config():
    return os.path.exists('/etc/lsyncd/lsyncd.conf.lua')


def check_service():
    try:
        check_output('/etc/init.d/kinopoisk-lsyncd status', shell=True)
        return True
    except CalledProcessError:
        return False


def check_pgrep():
    try:
        res = check_output('pgrep lsyncd', shell=True)
        return int(res)
    except CalledProcessError:
        return False


def check_process_ctime(pid):
    proc = psutil.Process(pid)
    if time.time() - proc.create_time() > THRESH_DAEMON_CTIME:
        return True
    else:
        return False


def exit_ok(extra=''):
    print('0; [OK] %s' % extra)
    sys.exit(0)


def exit_crit(extra=''):
    print('2; [CRIT] %s' % extra)
    sys.exit(2)


def main():
    if not check_is_active_master():
        exit_ok('nothing to do with inactive backoffice server')
    elif not check_config():
        exit_crit('cannot find lsyncd config on active backoffice')
    elif not check_service():
        exit_crit('daemon does not work correctely')
    pid = check_pgrep()
    if not pid:
        exit_crit('cannot check pgrep lsyncd')
    elif not check_process_ctime(pid):
        exit_crit('too young daemon process')
    else:
        exit_ok('works fine')


main()
