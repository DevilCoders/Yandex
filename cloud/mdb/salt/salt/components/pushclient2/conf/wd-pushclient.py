#!/usr/bin/env python3
"""
Pushclient watchdog
"""
import glob
import json
import os
import shlex
import subprocess
import time

BASEPATH = '/etc/pushclient'
STATUS_TEMPLATE = '/usr/bin/push-client -f -w -c {config} --status --json'


def config_discover(basepath):
    """
    Find all pushclient configs
    """
    return glob.glob('{path}/*.conf'.format(path=basepath))


def get_status(config):
    """
    Get status of pushclient for config
    """
    cmd = STATUS_TEMPLATE.format(config=config)
    tries_left = 3
    while tries_left > 0:
        proc = subprocess.Popen(
            ['/usr/bin/timeout', '-k', '7', '5'] + shlex.split(cmd), stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        stdout, stderr = proc.communicate()
        stdout = stdout.decode()
        stderr = stderr.decode()
        if not stderr and proc.poll() == 0:
            break
        if proc.poll() == 124:
            stderr = 'Status command timed out'
        tries_left -= 1
        time.sleep(1)
    return stdout, stderr


def restart():
    """
    Restart push-client service
    """
    subprocess.call(['/usr/sbin/service', 'statbox-push-client', 'restart'])


def _main():
    configs = config_discover(BASEPATH)
    for config in configs:
        out, err = get_status(config)
        if err:
            return restart()
        try:
            states = json.loads(out)
            for missing_state in [x for x in states if 'lag' not in x]:
                if os.path.exists(missing_state['name']) and os.path.getsize(missing_state['name']) == 0:
                    continue
                return restart()
            for state in [x for x in states if 'lag' in x]:
                if time.time() - state.get('last send time', 0) < 180 and state.get('commit delay', 0) > 3600:
                    return restart()
        except Exception as exc:
            print('{config} error: {exc}'.format(config=config, exc=repr(exc)))


if __name__ == '__main__':
    _main()
