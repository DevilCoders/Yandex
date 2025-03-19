#!/usr/bin/env python
"""
Check if local redis responds to ping
"""

import json
import os
import sys

from redis import StrictRedis

PORT = {{salt.pillar.get('data:redis:config:port', 6379)}}


def die(status=0, message='OK'):
    """
    Print status in monrun-compatible format
    """
    print('{status};{message}'.format(status=status, message=message))
    sys.exit(0)


def _main():
    main_notify_level = 2
    for address in ['127.0.0.1', '::1']:
        try:
            if os.path.exists('/etc/dbaas.conf'):
                with open('/etc/dbaas.conf') as dbaas_conf:
                    config = json.load(dbaas_conf)
                    hosts = config['cluster_hosts']
                    if len(hosts) == 1:
                        main_notify_level = 1
            with open(os.path.expanduser('~/.redispass')) as redispass:
                password = json.load(redispass)['password']
            conn = StrictRedis(host=address, port=PORT, db=0, password=password)
            info = conn.info('replication')
            message = 'OK, {}'.format(info['role'])
            die(message=message)
        except Exception as e:
            if str(e) == 'max number of clients reached':
                die(1, 'Max number of clients reached')
    die(main_notify_level, 'Local redis is dead')


if __name__ == '__main__':
    _main()
