#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
Script for deleting the garbage from storage (old WALs and leftover backup files)
"""

import argparse
import subprocess
import json

from pg_walg_backup_push import get_config, get_logger

try:
    from configparser import ConfigParser
except ImportError:
    from ConfigParser import ConfigParser


def delete_garbage(log, log_stream, timeout, s3_prefix):
    """
    Run wal-g
    """
    log.info('Starting wal-g delete garbage')

    command = [
        '/usr/bin/timeout',
        str(timeout),
        '/usr/bin/wal-g',
        '--config',
        '/etc/wal-g/wal-g.yaml',
        'delete',
        'garbage',
        '--confirm',
    ]

    if s3_prefix:
        command += ['--walg-s3-prefix', s3_prefix]

    subprocess.check_call(command, stdout=log_stream, stderr=log_stream, cwd='/tmp')


def main():
    """
    Console entry-point
    """
    parser = argparse.ArgumentParser()
    parser.add_argument('--s3-prefix', type=str, help="Custom S3 prefix path", required=False)
    parser.add_argument('-c', '--config', type=str, default='/etc/wal-g-backup-push.conf', help='config path')

    args = parser.parse_args()

    config = get_config(args.config)
    log, log_handler = get_logger(config)

    try:
        delete_garbage(log, log_handler.stream, config.get('main', 'timeout'), args.s3_prefix)
    except Exception as exc:
        log.exception('Unable to delete garbage')
        raise


if __name__ == '__main__':
    main()
