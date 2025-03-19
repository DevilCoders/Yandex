#!/usr/bin/env python
"""
Simple console tool to wait for zk to start
"""
from __future__ import print_function

import argparse
import socket
import sys
import time


NOT_SERVING = 'This ZooKeeper instance is not currently serving requests'
# Standalone is a valid state when we brew the image.
VALID_STATES = ('leader', 'follower', 'standalone')

def is_zk_alive(port):
    """
    Check if local zk is either leader or follower
    """
    line = None
    try:
        conn = socket.create_connection(('localhost', port), timeout=1)
        conn.send(b'mntr')
        data = conn.recv(65535)
        for line in data.split('\n'):
            if not line or line == NOT_SERVING:
                continue
            key, value = line.split('\t')
            if key == 'zk_server_state' and value in VALID_STATES:
                return True
    except Exception as exc:
        print(
            'line: "{}", error: {}'.format(line, exc), file=sys.stderr)
    return False


def _main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '-p', '--port', type=int, default=2181, help='zk port')
    parser.add_argument(
        '-w', '--wait', type=int, default=60, help='Time to wait')
    args = parser.parse_args()

    deadline = time.time() + args.wait
    while time.time() < deadline:
        if is_zk_alive(args.port):
            sys.exit(0)
        time.sleep(1)
    print('ZK is dead', file=sys.stderr)
    sys.exit(1)


if __name__ == '__main__':
    _main()
