#!/usr/bin/env python
"""
Check state of the local redis cluster instance
"""

import os
import json
import sys

from redis import StrictRedis

PORT = {{ salt.pillar.get('data:redis:config:port', 6379) }}


def die(status=0, message='OK'):
    """
    Print status in monrun-compatible format
    """
    print('{status};{message}'.format(status=status, message=message))
    sys.exit(0)


def _main():
    try:
        with open(os.path.expanduser('~/.redispass')) as redispass:
            password = json.load(redispass)['password']
        conn = StrictRedis(port=PORT, password=password)
        cluster_info = conn.cluster('info')
        state = cluster_info['cluster_state']
        if state == 'ok':
            die()
        assigned_slots = cluster_info['cluster_slots_assigned']
        if assigned_slots != '16384':
            die(2, '{} slots assigned'.format(assigned_slots))
        failed_slots = cluster_info['cluster_slots_fail']
        die(1, '{} slots failed'.format(failed_slots))
    except Exception:
        pass
    die(1, 'Local redis is dead')


if __name__ == '__main__':
    _main()
