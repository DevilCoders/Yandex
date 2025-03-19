#!/usr/bin/env python
"""
Check if local redis sentinel knows alive master
"""

import json
import os
import sys

from redis import StrictRedis

SENTINEL_PORT = {{ salt.pillar.get('data:sentinel:config:port', 26379) }}


def die(status=0, message='OK'):
    """
    Print status in monrun-compatible format
    """
    print('{status};{message}'.format(status=status, message=message))
    sys.exit(0)


def _main():
    main_notify_level = 2
    try:
        if os.path.exists('/etc/dbaas.conf'):
            with open('/etc/dbaas.conf') as dbaas_conf:
                config = json.load(dbaas_conf)
                master_name = config['cluster_name']
                hosts = config['cluster_hosts']
                if len(hosts) == 1:
                    main_notify_level = 1
        else:
            with open(os.path.expanduser('~/.redispass')) as redis_pass:
                parsed = json.load(redis_pass)
            master_name = parsed['master_name']
        sentinel = StrictRedis(port=SENTINEL_PORT)
        masters = sentinel.sentinel_masters()
        state = masters.get(master_name)
        if not state:
            die(main_notify_level, 'No master named {}'.format(master_name))
        if state['role-reported'] != 'master':
            die(main_notify_level, 'The sentinel monitors the wrong host')
        if state['is_odown']:
            die(main_notify_level, 'The master is objectively down')
        if state['is_sdown']:
            die(1, 'The master is subjectively down')
        die()
    except Exception as e:
        if str(e) == 'max number of clients reached':
            die(1, 'Max number of clients reached (Sentinel)')
    die(main_notify_level, 'Local sentinel is dead')


if __name__ == '__main__':
    _main()
