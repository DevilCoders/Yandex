#!/usr/bin/env python
"""
DBM heartbeat monitoring
"""

import json
import os
import sys

STATE_PATH = '/tmp/dom0_heartbeat.state'


def die(status=0, message='OK'):
    """
    Print status;message and exit
    """
    print('{status};{message}'.format(status=status, message=message))
    sys.exit(0)


def _main():
    if not os.path.exists(STATE_PATH):
        die(1, 'No state file')

    try:
        with open(STATE_PATH) as inp:
            data = json.load(inp)
        level = 0
        msg = []
        if not data['heartbeat']:
            level = 2
            msg.append('Host is dead')
        if data['warnings']:
            level = max(1, level)
            msg.append('{num} warnings'.format(num=len(data['warnings'])))
            msg += data['warnings']
        die(level, ', '.join(msg) if msg else 'OK')
    except Exception as exc:
        die(1, 'Unable to parse state: {error}'.format(error=repr(exc)))


if __name__ == '__main__':
    _main()
