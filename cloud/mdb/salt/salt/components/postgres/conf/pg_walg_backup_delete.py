#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
Script for deleting backup
"""

import argparse
import subprocess
import json

from pg_walg_backup_push import get_config, get_logger

try:
    from configparser import ConfigParser
except ImportError:
    from ConfigParser import ConfigParser


def delete_backup(log, log_stream, timeout, id, name, s3_prefix):
    """
    Run wal-g
    """
    log.info('Starting backup deletion')

    command = [
        '/usr/bin/timeout',
        str(timeout),
        '/usr/bin/wal-g',
        '--config',
        '/etc/wal-g/wal-g.yaml',
        'delete',
        'target',
        '--confirm',
    ]

    _add_target(command, id, name)
    _add_s3_prefix(command, s3_prefix)

    subprocess.check_call(command, stdout=log_stream, stderr=log_stream, cwd='/tmp')


def _add_target(command, id, name):
    if id:
        command += ['--target-user-data', json.dumps({'backup_id': id})]
    elif name:
        command += [name]
    else:
        raise Exception("either name or id of backup should be specified")


def _add_s3_prefix(command, s3_prefix):
    if s3_prefix:
        command += ['--walg-s3-prefix', s3_prefix]


def main():
    """
    Console entry-point
    """
    parser = argparse.ArgumentParser()
    parser.add_argument('--id', type=str, help="Backup id for removing", required=False)
    parser.add_argument('--name', type=str, help="Backup name for removing", required=False)
    parser.add_argument('--s3-prefix', type=str, help="Custom S3 prefix path", required=False)
    parser.add_argument('-c', '--config', type=str, default='/etc/wal-g-backup-push.conf', help='config path')

    args = parser.parse_args()

    config = get_config(args.config)
    log, log_handler = get_logger(config)

    try:
        delete_backup(log, log_handler.stream, config.get('main', 'timeout'), args.id, args.name, args.s3_prefix)
    except Exception as exc:
        log.exception('Unable to delete backup')
        raise


if __name__ == '__main__':
    main()
