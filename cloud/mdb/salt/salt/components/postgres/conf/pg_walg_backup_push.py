#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
Script for making backup from needed host
"""

import argparse
import datetime
import logging
import logging.handlers
import os
from random import randint
import socket
import subprocess
import time
import json

import psycopg2
from kazoo.client import KazooClient

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
    Get status of local postgresql host
    """
    info = {
        'alive': False,
        'replica': False,
    }
    try:
        conn = psycopg2.connect('dbname=postgres')
        cur = conn.cursor()
        cur.execute('SELECT pg_is_in_recovery()')
        info['alive'] = True
        if cur.fetchone()[0]:
            info['replica'] = True
            cur.execute('SELECT extract(epoch FROM ' '(current_timestamp - ts)) FROM repl_mon')
            if cur.fetchone()[0] > config.getint('main', 'max_replication_lag'):
                info['alive'] = False
    except Exception as exc:
        log.error('PostgreSQL is dead: %s', repr(exc))

    return info


def get_pgsync_role(zk_client, hostname, config):
    """
    Get role of hostname using pgsync
    """
    pgsync_prefix = config.get('main', 'pgsync_prefix')
    leader_lock = zk_client.Lock(os.path.join(pgsync_prefix, 'leader'))
    sync_lock = zk_client.Lock(os.path.join(pgsync_prefix, 'sync_replica'))
    quorum_lock = zk_client.Lock(os.path.join(pgsync_prefix, 'quorum', 'members', hostname))
    leader_contenders = leader_lock.contenders()
    sync_contenders = sync_lock.contenders()
    quorum_contenders = quorum_lock.contenders()
    if leader_contenders and leader_contenders[0] == hostname:
        return 'master'
    elif quorum_contenders and quorum_contenders[0] == hostname:
        return 'quorum'
    elif sync_contenders and sync_contenders[0] == hostname:
        return 'sync'
    return 'async'


def election(log, config, pg_info):
    """
    Participate in zk-based election
    """
    try:
        deadline = time.time() + config.getint('main', 'election_timeout')
        log.info('Starting election with deadline: %s', deadline)
        hostname = socket.getfqdn()
        zk_client = KazooClient(config.get('main', 'zk_hosts'))
        zk_client.start()
        elected = False
        election_lock = zk_client.Lock(config.get('main', 'election_lock'), hostname)
        if config.getboolean('main', 'use_pgsync'):
            role = get_pgsync_role(zk_client, hostname, config)
        else:
            role = 'async' if pg_info['replica'] else 'master'
        log.info('Local host role is %s', role)
        while time.time() < deadline:
            contenders = election_lock.contenders()
            if not contenders or (role in ('async', 'quorum') and hostname not in contenders):
                log.info('Acquiring election lock')
                election_lock.acquire(timeout=10)
            else:
                if len(contenders) > 1 and contenders[0] == hostname:
                    if role in ('master', 'sync'):
                        log.info('We are %s: releasing election lock', role)
                        election_lock.release()
                        elected = False
                        continue
            time.sleep(1)
            if contenders and contenders[0] == hostname:
                if len(contenders) == 1:
                    elected = True
                elif len(contenders) > 1:
                    log.info('Waiting for other nodes to stop election: %s', ', '.join(contenders[1:]))
        return elected
    except Exception as exc:
        log.error('election error: %s', repr(exc))
        return False


def add_id_to_command(command, id):
    if id:
        command += [
            '--add-user-data',
            json.dumps({'backup_id': id}),
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
    pgdata,
    log_stream,
    timeout,
    user_backup,
    id=None,
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
        pgdata,
        '--config',
        '/etc/wal-g/wal-g.yaml',
    ]
    add_id_to_command(command, id)

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
            'FIND_FULL',
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
    parser.add_argument(
        '--skip-election-in-zk',
        action='store_true',
        help="Don't participate in ZK election",
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

    args = parser.parse_args()

    config = get_config(args.config)
    log, log_handler = get_logger(config)

    if args.delta_from_previous_backup and args.delta_from_id:
        log.error('Can\'t use both of "--delta-from-previous-backup", "--delta-from-id" flags.')
        return
    try:
        info = get_pg_info(log, config)
        if not info['alive']:
            log.info('Not participating in election')
            return
        if not args.skip_election_in_zk:
            if not election(log, config, info):
                log.info('Election lost. Skipping backup')
                return
            sleep_seconds = randint(0, config.getint('main', 'sleep'))
            log.info('Sleep %d seconds', sleep_seconds)
            time.sleep(sleep_seconds)
        do_backup(
            log,
            config.get('main', 'pgdata'),
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
