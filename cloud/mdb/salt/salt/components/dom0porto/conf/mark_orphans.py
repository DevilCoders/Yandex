#!/usr/bin/env python
"""
Mark orphan containers to prevent porto-agent from starting them
"""

import json
import os
import subprocess
import time

MAX_TRIES = 3
RETRY_WAIT = 5


def get_containers():
    """
    Get expected container list from pillar
    """
    tries = 0
    while True:
        try:
            out = subprocess.check_output(
                [
                    '/usr/bin/timeout',
                    '30',
                    '/usr/bin/salt-call',
                    'pillar.get',
                    'data:porto',
                    '--out=json',
                ],
            )
            data = json.loads(out)
            return {
                key: value for key, value in data['local'].items() if not value['container_options']['pending_delete']
            }
        except Exception:
            tries += 1
            if tries > MAX_TRIES:
                raise
            time.sleep(RETRY_WAIT)


def mark_orphans(expected_containers):
    """
    Traverse local container state and mark missing from expected
    """
    base = '/var/cache/porto_agent_states'
    existing = []
    for filename in os.listdir(base):
        if filename.endswith('.json'):
            container = os.path.splitext(filename)[0]
            existing.append(container)

    for container in existing:
        if container not in expected_containers:
            extra_path = os.path.join(base, '{container}.extra'.format(container=container))
            if not os.path.exists(extra_path):
                tmp_path = '{extra_path}.tmp'.format(extra_path=extra_path)
                with open(tmp_path, 'w') as out:
                    json.dump({'operation': 'mark-orphan'}, out)
                os.rename(tmp_path, extra_path)


def _main():
    mark_orphans(get_containers())


if __name__ == '__main__':
    _main()
