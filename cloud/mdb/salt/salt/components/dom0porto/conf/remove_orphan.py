#!/usr/bin/env python
"""
Drop container marked orphan
"""

import argparse
import json
import logging
import os
import time

logging.basicConfig(level=logging.INFO, format='%(asctime)s %(levelname)s:\t%(message)s')
LOG = logging.getLogger('move-container')

BASE = '/var/cache/porto_agent_states'


def get_state_path(name):
    """
    Get path to state file
    """
    return os.path.join(BASE, '{name}.json'.format(name=name))


def get_extra_path(name):
    """
    Get path to extra state file
    """
    return os.path.join(BASE, '{name}.extra'.format(name=name))


def run_checks(name, force):
    """
    Check if drop is safe
    """
    extra_path = get_extra_path(name)
    if os.path.exists(extra_path):
        data = {}
        with open(extra_path) as extra_file:
            try:
                data = json.load(extra_file)
            except json.JSONDecodeError as exc:
                if force:
                    LOG.error('Unable to load extra file: %r', exc)
                else:
                    raise
        if data.get('operation') != 'mark-orphan' and not force:
            raise RuntimeError('{name} is not marked as orphan'.format(name=name))
    elif not force:
        raise RuntimeError('No extra file for {name}'.format(name=name))


def get_volumes(name):
    """
    Get volumes from porto agent state
    """
    state_path = get_state_path(name)
    if os.path.exists(state_path):
        with open(state_path) as state_file:
            data = json.load(state_file)
        return data['volumes']
    return []


def drop_container(name, force):
    """
    Drop orphan container
    """
    run_checks(name, force)
    for volume in get_volumes(name):
        if os.path.exists(volume['dom0_path']):
            bkup_path = '{path}.{ts}.bak'.format(path=volume['dom0_path'], ts=int(time.time()))
            LOG.info('moving %s to %s', volume['dom0_path'], bkup_path)
            os.rename(volume['dom0_path'], bkup_path)
    state_path = get_state_path(name)
    extra_path = get_extra_path(name)

    for path in [state_path, extra_path]:
        if os.path.exists(path):
            LOG.info('removing %s', path)
            os.unlink(path)


def _main():
    parser = argparse.ArgumentParser()
    parser.add_argument('container', help='container to drop')
    parser.add_argument('-f', '--force', action='store_true', help='skip safety checks')
    args = parser.parse_args()

    drop_container(args.container, args.force)


if __name__ == '__main__':
    _main()
