#!/opt/yandex/pgsync/bin/python3.6

from __future__ import absolute_import, print_function, unicode_literals

import json
import os
import subprocess
import sys
import time

from cloud.mdb.pgsync import read_config


def restart(comment=None):
    subprocess.call('/etc/init.d/pgsync restart', shell=True, stdout=sys.stdout, stderr=sys.stderr)
    print('Pgsync has been restarted due to %s.' % comment)
    sys.exit(0)


def rewind_running():
    pids = [pid for pid in os.listdir('/proc') if pid.isdigit()]
    for pid in pids:
        try:
            cmd = open(os.path.join('/proc', pid, 'cmdline'), 'rb').read()
            if 'pg_rewind' in cmd:
                return True
        except IOError:
            # proc has already terminated
            continue
    return False


def main():
    config = read_config(filename='/etc/pgsync.conf')
    work_dir = config.get('global', 'working_dir')
    stop_file = os.path.join(work_dir, 'pgsync.stopped')

    if os.path.exists(stop_file):
        print('Pgsync has been stoppped gracefully. Not doing anything.')
        sys.exit(0)

    p = subprocess.call('/etc/init.d/pgsync status', shell=True, stdout=sys.stdout, stderr=sys.stderr)
    if p != 0:
        restart('dead service')

    status_file = os.path.join(work_dir, 'pgsync.status')
    # We multiply on 3 because sanity checks and pg_rewind may take
    # some time without updating status-file
    timeout = config.getint('replica', 'recovery_timeout') * 3
    f = open(status_file, 'r')
    state = json.loads(f.read())
    f.close()
    if float(state['ts']) <= time.time() - timeout and not rewind_running():
        restart('stale info in status-file')


if __name__ == '__main__':
    main()
