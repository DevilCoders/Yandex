#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
Script for making backup from needed host
"""

import argparse
import datetime
import logging
import logging.handlers
from random import randint
import subprocess
import time
import json
import random
import string

import psycopg2

try:
    from configparser import ConfigParser
except ImportError:
    from ConfigParser import ConfigParser


def get_config(path):
    """
    Parse config from path
    """
    config = ConfigParser()
    config.read(path)
    return config


def get_logger(config):
    """
    Initialize logger
    """
    log = logging.getLogger('main')
    log.setLevel(logging.DEBUG)
    log_handler = logging.handlers.WatchedFileHandler(
        config.get('main', 'log_path'),
        mode='a',
        encoding=None,
        delay=False,
    )
    log_handler.setFormatter(logging.Formatter('%(asctime)s [%(levelname)s]: %(message)s'))
    log.addHandler(log_handler)

    return log, log_handler


def get_pg_info(log, config):
    """
    Get status of local greenplum host
    """
    info = {
        'alive': False,
        'replica': False,
    }
    gp_admin = config.get('main', 'gp_admin')
    try:
        conn = psycopg2.connect('user=%s dbname=postgres' % gp_admin)
        cur = conn.cursor()
        cur.execute('SELECT pg_is_in_recovery()')
        info['alive'] = True
        if cur.fetchone()[0]:
            info['replica'] = True
    except Exception as exc:
        log.error('Greenplum is dead: %s', repr(exc))

    return info


def add_backup_id_to_command(command, backup_id):
    if backup_id:
        command += [
            '--add-user-data',
            json.dumps({'backup_id': backup_id}),
        ]


def add_full_flag_to_command(command, is_full):
    if is_full:
        command += ['--full']


def add_previous_backup_to_command(command, from_id):
    """
    Add info about previous backup, if current one is delta-backup
        or flag --full, if current one is full-backup
    """
    if from_id:
        command += [
            '--delta-from-user-data',
            json.dumps({'backup_id': from_id}),
        ]
        # ignore max delta steps from config if we are explicitly asked to create delta backup
        command += [
            '--walg-delta-max-steps',
            str(10 ** 9),
        ]
    else:
        add_full_flag_to_command(command, True)


def add_from_name_backup_to_command(command, from_name):
    if from_name:
        command += [
            '--delta-from-name',
            from_name,
        ]
        # ignore max delta steps from config if we are explicitly asked to create delta backup
        command += [
            '--walg-delta-max-steps',
            str(10 ** 9),
        ]
    else:
        add_full_flag_to_command(command, True)


def add_permanent_flag_to_command(command, user_backup, log, id):
    if user_backup:
        if not id:
            log.error('User backup should have id')
            return False
        # user backups should be permanent
        command += [
            '--permanent',
        ]
    return True


def do_backup(
    log,
    log_stream,
    timeout,
    user_backup,
    backup_id=None,
    from_id=None,
    is_full=False,
    delta_from_previous_backup=True,
    from_name=None,
):
    """
    Run wal-g
    """
    command = [
        '/usr/bin/timeout',
        str(timeout),
        '/usr/bin/wal-g',
        'backup-push',
        '--config',
        '/etc/wal-g/wal-g.yaml',
    ]

    if not backup_id:
        # for cron/manual backups, generate some unique id, it will be useful in the future (in backup-api)
        backup_id = 'cron' + ''.join(random.choice(string.ascii_lowercase + string.digits) for _ in range(23))

    add_backup_id_to_command(command, backup_id)

    if from_id:
        # if previous backup name or user-data not specified, wal-g will make delta backup from previous backup,if possible.
        if not delta_from_previous_backup:
            add_previous_backup_to_command(command, from_id)
    elif from_name:
        if not delta_from_previous_backup:
            add_from_name_backup_to_command(command, from_name)
    else:
        add_full_flag_to_command(command, is_full)

    if not add_permanent_flag_to_command(command, user_backup, log, id):
        return

    log.info('Starting backup')
    log.info(' '.join(command))
    subprocess.check_call(command, stdout=log_stream, stderr=log_stream, cwd='/tmp')


def delete_old_backups(log, keep, log_stream):
    """
    Delete old backups up to keep num
    """
    deleted_before = datetime.datetime.utcnow() - datetime.timedelta(days=keep)
    log.info('Delete old backups')

    subprocess.check_call(
        [
            '/usr/bin/timeout',
            '86400',
            '/usr/bin/wal-g',
            'delete',
            'before',
            deleted_before.strftime('%Y-%m-%dT%H:%M:%SZ'),
            '--confirm',
            '--config',
            '/etc/wal-g/wal-g.yaml',
        ],
        stdout=log_stream,
        stderr=log_stream,
        cwd='/tmp',
    )


def main():
    """
    Console entry-point
    """
    parser = argparse.ArgumentParser()
    parser.add_argument('-c', '--config', type=str, default='/etc/wal-g-backup-push.conf', help='config path')
    parser.add_argument(
        '--skip-delete-old',
        action='store_true',
        help="Don't delete old backups",
    )
    parser.add_argument('--id', type=str, help="Backup id")
    parser.add_argument('--delta-from-id', type=str, help="Delta from specified backup id")
    parser.add_argument('--delta-from-name', type=str, help="Delta from specified backup name")
    parser.add_argument(
        '--user-backup',
        action='store_true',
        help='Is backup created by user (from backup api)',
    )
    parser.add_argument('--full', action='store_true')
    parser.add_argument('--delta-from-previous-backup', action='store_true')

    parser.add_argument('--no-sleep', action='store_true')

    args = parser.parse_args()

    config = get_config(args.config)
    log, log_handler = get_logger(config)

    if args.delta_from_previous_backup and args.delta_from_id:
        log.error('Can\'t use both of "--delta-from-previous-backup", "--delta-from-id" flags.')
        return
    try:
        info = get_pg_info(log, config)
        if not info['alive']:
            log.info('Not participating in election (not alive)')
            return

        if info['replica']:
            log.info('Not participating in election (not master)')
            return
        if not args.no_sleep:
            sleep_seconds = randint(0, config.getint('main', 'sleep'))
            log.info('Sleep %d seconds', sleep_seconds)
            time.sleep(sleep_seconds)

        do_backup(
            log,
            log_handler.stream,
            config.get('main', 'timeout'),
            args.user_backup,
            args.id,
            args.delta_from_id,
            args.full,
            args.delta_from_previous_backup,
            args.delta_from_name,
        )
        if not args.skip_delete_old:
            delete_old_backups(log, config.getint('main', 'keep'), log_handler.stream)
    except Exception as exc:
        log.exception('Unable to make backup')
        raise


if __name__ == '__main__':
    main()
