#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
Script for restore point creation
"""

import argparse
import subprocess
import random
import string

from gp_walg_backup_push import get_config, get_logger, get_pg_info


def create_restore_point(log, log_stream, timeout, restore_point_id):
    """
    Run wal-g create-restore-point
    """
    if not restore_point_id:
        # for cron/manual restore points, generate some unique id, it will be useful in the future (in backup-api)
        restore_point_id = 'cron' + ''.join(random.choice(string.ascii_lowercase + string.digits) for _ in range(23))

    command = [
            '/usr/bin/timeout',
            str(timeout),
            '/usr/bin/wal-g',
            'create-restore-point',
            restore_point_id,
            '--config',
            '/etc/wal-g/wal-g.yaml',
    ]

    log.info('Starting restore point creation')
    log.info(' '.join(command))
    subprocess.check_call(
        command,
        stdout=log_stream,
        stderr=log_stream,
        cwd='/tmp')


def main():
    """
    Console entry-point
    """
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '-c',
        '--config',
        type=str,
        default='/etc/wal-g-create-restore-point.conf',
        help='config path')
    parser.add_argument(
        '--id',
        type=str,
        help="restore point id"
    )

    args = parser.parse_args()

    config = get_config(args.config)
    log, log_handler = get_logger(config)

    try:
        info = get_pg_info(log, config)
        if not info['alive']:
            log.info('Not participating in election (not alive)')
            return

        if info['replica']:
            log.info('Not participating in election (not master)')
            return

        create_restore_point(
            log,
            log_handler.stream,
            config.get('main', 'timeout'),
            args.id,
        )
    except Exception as exc:
        log.exception('Unable to make restore point')
        raise


if __name__ == '__main__':
    main()
