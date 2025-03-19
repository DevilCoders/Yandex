#!/usr/bin/env python
# -*- coding: utf-8 -*-

import argparse
import datetime
import json
import logging
import logging.handlers
import os.path
import subprocess
import time
from contextlib import closing
from random import randint

import MySQLdb
import MySQLdb.cursors as cursors
from kazoo.client import KazooClient
from kazoo.handlers.threading import SequentialThreadingHandler

try:
    from configparser import ConfigParser
except ImportError:
    from ConfigParser import ConfigParser

MAX_LAG_FOR_BACKUP_HOST = 6 * 60 * 60


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
    log.setLevel(logging.INFO)
    log_handler = logging.handlers.RotatingFileHandler(
        config.get('main', 'log_path'), mode='a', maxBytes=10 * 1024 * 1024, backupCount=4, encoding=None, delay=0
    )
    log_handler.setFormatter(logging.Formatter('%(asctime)s [%(levelname)s]: %(message)s'))
    log_handler.setLevel(logging.INFO)
    log.addHandler(log_handler)

    return log, log_handler


def do_backup(log, log_stream, datadir, timeout, user_backup, backup_id=None):
    """
    Run wal-g
    """
    log.info('Starting backup')
    walg_cmd = [
        '/usr/bin/timeout',
        str(timeout),
        '/usr/bin/wal-g-mysql',
        'backup-push',
        '--config',
        '/etc/wal-g/wal-g.yaml',
    ]

    if backup_id:
        walg_cmd += ['--add-user-data', json.dumps({'backup_id': backup_id})]

    if user_backup:
        walg_cmd += ['--permanent']

    subprocess.check_call(walg_cmd, stdout=log_stream, stderr=log_stream, cwd='/tmp')


def delete_old_backups(log, log_stream, keep):
    """
    Delete old backups up to keep num
    """
    deleted_before = datetime.datetime.utcnow() - datetime.timedelta(days=keep)
    log.info('Delete old backups')

    walg_cmd = [
        '/usr/bin/timeout',
        '7200',
        '/usr/bin/wal-g-mysql',
        'delete',
        'before',
        'FIND_FULL',
        deleted_before.strftime('%Y-%m-%dT%H:%M:%SZ'),
        '--confirm',
        '--config',
        '/etc/wal-g/wal-g.yaml',
    ]

    subprocess.check_call(walg_cmd, stdout=log_stream, stderr=log_stream, cwd='/tmp')


def do_backup_delete(log, log_stream, target):
    log.info('Delete target backup %s', target)

    walg_cmd = [
        '/usr/bin/timeout',
        '3600',
        '/usr/bin/wal-g-mysql',
        '--config',
        '/etc/wal-g/wal-g.yaml',
        'delete', 'target', target,
        '--confirm',
    ]

    subprocess.check_call(walg_cmd, stdout=log_stream, stderr=log_stream, cwd='/tmp')


def do_binlog_backup(log, log_stream):
    """
    Run wal-g binlog-push
    """
    log.info('Starting binlog upload')
    walg_cmd = [
        '/usr/bin/wal-g-mysql',
        'binlog-push',
        '--config',
        '/etc/wal-g/wal-g.yaml',
    ]

    subprocess.check_call(walg_cmd, stdout=log_stream, stderr=log_stream, cwd='/tmp')


def is_master(defaults_file):
    with closing(MySQLdb.connect(db='mysql', read_default_file=defaults_file, cursorclass=cursors.DictCursor)) as conn:
        return _is_master(conn)


def _is_master(conn):
    cur = conn.cursor()
    cur.execute("SHOW SLAVE STATUS")
    return not len(cur.fetchall()) > 0


def _is_offline(conn):
    cur = conn.cursor()
    cur.execute("SELECT @@offline_mode")
    return int(cur.fetchone()[0]) > 0


def _is_read_only(conn):
    cur = conn.cursor()
    cur.execute("SELECT @@read_only")
    return int(cur.fetchone()[0]) > 0


def set_parallel_workers(log, defaults_file, workers):
    with closing(MySQLdb.connect(db='mysql', read_default_file=defaults_file, cursorclass=cursors.DictCursor)) as conn:
        cur = conn.cursor()
        cur.execute("SELECT @@slave_parallel_workers AS current_workers")
        workers = int(workers)
        current_workers = int(cur.fetchone()['current_workers'])
        if current_workers != workers:
            log.info("setting slave_parallel_workers to {}".format(int(workers)))
            cur.execute("SET GLOBAL slave_parallel_workers = {}".format(workers))
            cur.execute("STOP SLAVE SQL_THREAD")
            cur.execute("START SLAVE SQL_THREAD")
        return current_workers


def do_backup_with_safe_replication(log, log_stream, datadir, timeout, defaults_file, user_backup, backup_id=None):
    if is_master(defaults_file):
        do_backup(log, log_stream, datadir, timeout, user_backup, backup_id)
        return
    old_workers = None
    try:
        old_workers = set_parallel_workers(log, defaults_file, 0)
        do_backup(log, log_stream, datadir, timeout, user_backup, backup_id)
    finally:
        if old_workers is not None:
            set_parallel_workers(log, defaults_file, old_workers)


def connect_to_zk(zk_hosts, log):
    retry_options = {'max_tries': 0, 'delay': 0, 'backoff': 1, 'max_jitter': 0, 'max_delay': 1}
    zk_client = KazooClient(
        hosts=zk_hosts,
        handler=SequentialThreadingHandler(),
        timeout=1,
        connection_retry=retry_options,
        command_retry=retry_options,
        logger=log,
    )
    zk_client.start()
    return zk_client


def is_host_for_backups(log, zk_client, config, defaults_file):
    cid = config.get('main', 'cid')
    fqdn = config.get('main', 'fqdn')
    skip_crashed_hosts = config.getboolean('main', 'skip_crashed_hosts')
    if zk_client.exists('/mysql/{}/maintenance'.format(cid)):
        log.info("don't make backups during maintenance")
        return False
    if not zk_client.exists('/mysql/{}/active_nodes'.format(cid)):
        log.info("there is no node active_nodes")
        if is_master(defaults_file):
            # MDB-13539. When all HA-replicas lost - make backup from master:
            log.info("current host is master")
            return True
        return False
    value, _ = zk_client.get('/mysql/{}/active_nodes'.format(cid))
    active_nodes = json.loads(value)
    candidates = {}
    master_according_to_zk = None
    for host in active_nodes:
        health_path = '/mysql/{}/health/{}'.format(cid, host)
        if not zk_client.exists(health_path):
            log.info("there is no health node for {}".format(host))
            return False
        value, _ = zk_client.get('/mysql/{}/health/{}'.format(cid, host))
        info = json.loads(value)
        if info['is_master']:
            master_according_to_zk = host
            continue
        lag = info['slave_state']['replication_lag']
        if lag > MAX_LAG_FOR_BACKUP_HOST:
            log.info('host {} has too big lag: {}, skipping'.format(host, lag))
            continue
        if zk_client.exists('/mysql/{}/recovery/{}'.format(cid, host)):
            log.info('host {} is in recovery, skipping'.format(host))
            continue
        after_crash = info.get('daemon_state', {}).get('crash_recovery', False)
        if after_crash and skip_crashed_hosts:
            log.info('host {} is after crash recovery, skipping'.format(host))
            continue
        candidates[host] = lag
    log.info("candidates: {}".format(candidates))
    candidates = list(sorted(candidates, key=lambda hostname: candidates[hostname]))
    host_is_master = is_master(defaults_file)
    log.info("current master is: {}".format(fqdn if host_is_master else master_according_to_zk))
    if len(candidates) == 0:
        log.info("main only candidate is master")
        return host_is_master
    else:
        log.info("main candidate is: {}".format(candidates[0]))
        return fqdn == candidates[0]


def main():
    """
    Console entry-point
    """
    parser = argparse.ArgumentParser()
    parser.add_argument('-c', '--config', type=str, default='/etc/wal-g-backup-push.conf', help='config path')
    parser.add_argument('-d', '--datadir', type=str, default='/var/lib/mysql', help='mysql datadir path')
    parser.add_argument(
        '--defaults-file',
        type=str,
        default=os.path.expanduser("~/.my.cnf"),
        help='path for my.cnf with login/password for cluster',
    )
    subparsers = parser.add_subparsers(dest='command')
    parser_backup = subparsers.add_parser('backup_push', help='run full backup with wal-g backup-push')
    parser_backup.add_argument(
        '--skip-election-in-zk',
        action='store_true',
        help="Don't participate in ZK elections",
    )
    parser_backup.add_argument(
        '--skip-sleep',
        action='store_true',
        help="Don't sleep",
    )
    parser_backup.add_argument(
        '--skip-delete-old',
        action='store_true',
        help="don't use, it's for back compat",
    )
    parser_backup.add_argument(
        '--backup_id',
        default=None,
        help="Backup id",
    )
    parser_backup.add_argument(
        '--user-backup',
        action='store_true',
        help='Is backup created by user',
    )
    parser_binlogs_push = subparsers.add_parser('binlogs_push', help='run binlog backup with wal-g mysql-cron')
    parser_delete = subparsers.add_parser('delete', help='delete target backup and unused binlogs')
    parser_delete.add_argument(
        '--target',
        type=str,
        help='target backup name',
    )
    parser_delete.add_argument(
        '--skip-delete-old',
        action='store_true',
        help="Don't delete old backups",
    )


    args = parser.parse_args()
    config = get_config(args.config)
    log, log_handler = get_logger(config)
    zk_hosts = config.get('main', 'zk_hosts').split(',')
    lock = None
    zk_client = None
    try:
        if args.command == 'backup_push':
            if not args.skip_election_in_zk:
                zk_client = connect_to_zk(zk_hosts, log)
                log.info("Connected to zk")
                if not is_host_for_backups(log, zk_client, config, args.defaults_file):
                    log.info("It's not a backup host")
                    return
                log.info("It's a backup host")
                lock = zk_client.Lock('/mysql/{}/backup_lock'.format(config.get('main', 'cid')))
                if not lock.acquire(blocking=False):
                    log.info("Backup lock not acquired")
                    return
                log.info("Backup lock acquired")
            if not args.skip_sleep:
                sleep_seconds = randint(0, config.getint('main', 'sleep'))
                log.info('Sleep %d seconds', sleep_seconds)
                time.sleep(sleep_seconds)
            do_backup_with_safe_replication(
                log, log_handler.stream, args.datadir, config.getint('main', 'timeout'), args.defaults_file, args.user_backup, args.backup_id,
            )
            log.info("Backup done")
        elif args.command == 'binlogs_push':
            with closing(MySQLdb.connect(db='mysql', read_default_file=args.defaults_file)) as conn:
                if not _is_master(conn):
                    log.info("binlogs_push: Not a master. Do nothing.")
                    return
                if _is_offline(conn):
                    log.info("binlogs_push: MySQL is in offline_mode. Do nothing.")
                    return
                if _is_read_only(conn):
                    log.info("binlogs_push: MySQL is in read_only. Do nothing.")
                    return
            do_binlog_backup(log, log_handler.stream)
        elif args.command == 'delete':
            do_backup_delete(log, log_handler.stream, args.target)
            if not args.skip_delete_old:
                delete_old_backups(log, log_handler.stream, config.getint('main', 'keep'))
        else:
            raise Exception("unexpected command")
    except Exception as e:
        log.exception('Unable to %s backup: %s', args.command, e)
        raise
    finally:
        if lock is not None:
            lock.release()
            log.info("Backup lock released")
        if zk_client is not None:
            zk_client.stop()
            log.info("zk_client stopped")


if __name__ == '__main__':
    main()
