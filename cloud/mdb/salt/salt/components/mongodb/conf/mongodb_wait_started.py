#!/usr/bin/env python
"""
Simple console tool to wait for mongodb to start
"""

import argparse
import json
import subprocess
import sys
import time

SRV_TO_CHECK = {
    'mongod': 'mongodb_ping',
    'mongodb': 'mongodb_ping',
    'mongos': 'mongos_ping',
    'mongocfg': 'mongocfg_up',
}


def is_mongodb_alive(srv='mongodb'):
    """
    Check if local mongodb is alive
    """
    try:
        out = subprocess.check_output([
            'timeout',
            '5',
            'sudo',
            '-u',
            'monitor',
            'monrun',
            '-r',
            SRV_TO_CHECK[srv],
            '-f',
            'json',
        ])
        data = json.loads(out)
        if data['events'][0]['status'] == 'OK':
            return True
    except Exception as exc:
        print(repr(exc))
    return False


def _main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '-w', '--wait', type=int, default=60, help='Time to wait')
    parser.add_argument(
        '-s', '--srv', choices=['mongodb', 'mongos', 'mongocfg'], default='mongodb', help='MongoDB service to wait [mongodb(default), mongos, mongocfg]')
    args = parser.parse_args()

    deadline = time.time() + args.wait
    while time.time() < deadline:
        if is_mongodb_alive(args.srv):
            sys.exit(0)
        time.sleep(1)
    print('MongoDB is dead')
    sys.exit(1)


if __name__ == '__main__':
    _main()
