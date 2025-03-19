#!/usr/bin/env python3.5
"""
Selective env variable pass for infra tests
"""

import json
import os
import subprocess
import sys


def get_max_timestamp(topic, branch, port, host):
    """
    Get latest available timestamp for topic
    """
    cmd = [
        'timeout',
        '5',
        'ssh',
        '-p',
        port,
        host,
        'gerrit',
        'query',
        '--current-patch-set',
        '--format=JSON',
        'status:open',
        'topic:{topic}'.format(topic=topic),
        'branch:{branch}'.format(branch=branch),
    ]
    output = subprocess.check_output(cmd)
    data = [json.loads(x.decode('utf-8')) for x in output.splitlines() if x]
    max_timestamp = 0
    for i in data:
        timestamp = i.get('currentPatchSet', {}).get('createdOn', 0)
        if timestamp > max_timestamp:
            max_timestamp = timestamp

    return max_timestamp


def _main():
    topic = os.environ.get('GERRIT_TOPIC')
    branch = os.environ.get('GERRIT_BRANCH')
    max_ts = get_max_timestamp(topic, branch, os.environ.get('GERRIT_PORT'),
                               os.environ.get('GERRIT_HOST'))
    if not topic:
        print('ERROR: Empty topic')
        sys.exit(1)
    if not branch:
        print('ERROR: Empty base branch')
        sys.exit(1)
    with open('infra.properties', 'w') as out:
        out.writelines([
            'GERRIT_TOPIC={topic}\n'.format(topic=topic),
            'GERRIT_BRANCH={branch}\n'.format(branch=branch),
            'MAX_SEEN_TS={ts}\n'.format(ts=max_ts),
        ])


if __name__ == '__main__':
    _main()
