#!/usr/bin/env python
# coding: utf-8

from __future__ import print_function

import hashlib
import json
import logging
import os.path
import subprocess
import threading
import time
from argparse import ArgumentParser
from datetime import datetime

import psycopg2
import psycopg2.extensions

log = logging.getLogger(__name__)


class Postgres(object):
    """
    Postgres helpers
    """
    restore_port = 3432

    def __init__(self, pg_bin, pg_data, pg_config, pg_version):
        self.bin = pg_bin
        self.data = pg_data
        self.config = pg_config
        self.version = pg_version

    def make_pg_cmd(self, cmd, *cmd_args):
        return [os.path.join(self.bin, cmd)] + list(cmd_args)

    @classmethod
    def make_restore_dsn(cls):
        return 'dbname=postgres port=%d' % cls.restore_port

    def path_to_pg_file(self, file_path):
        return os.path.join(self.data, file_path)


def run_command(cmd_args):
    log.info('Execute %s', cmd_args)
    cmd = subprocess.Popen(
        cmd_args,
        stderr=subprocess.PIPE,
        stdout=subprocess.PIPE,
    )
    cmd.wait()

    if cmd.returncode is not None:
        log.info('Out: %s', cmd.stdout.read())
    err_out = cmd.stderr.read().strip()
    if err_out:
        log.warning('Err: %s', err_out)
    log.log(logging.INFO if cmd.returncode == 0 else logging.WARNING, 'command exit with %r code', cmd.returncode)
    return cmd.returncode == 0


def run_restore(pg):
    postgre_options = [
        '-p',
        str(pg.restore_port),
        '-c',
        'config_file=%s' % pg.config,
        '-c',
        'archive_mode=off',
        '-c',
        'hot_standby=off',
        '-c',
        # 1000 is enough in most cases
        'max_prepared_transactions=1000',
        '-c',
        'synchronous_commit=local',
        '-c',
        'log_min_messages=PANIC',
        '-c',
        'log_min_error_statement=PANIC',
    ]
    cmd_args = pg.make_pg_cmd(
        'pg_ctl',
        'start',
        '-D',
        pg.data,
        '-w',  # wait until operation completes
        '-o',
        ' '.join(postgre_options),
    )
    run_command(cmd_args)


def stop_restored_postgres(pg):
    cmd_args = pg.make_pg_cmd(
        'pg_ctl',
        'stop',
        '-D',
        pg.data,
        '-w',
    )
    run_command(cmd_args)


def is_postgres_ready(pg, old_postgres_password=None):
    dsn = pg.make_restore_dsn()
    try:
        conn = psycopg2.connect(dsn)
        log.debug('Connected to postgres')
    except psycopg2.OperationalError as exc:
        if old_postgres_password is not None:
            try:
                conn = psycopg2.connect(dsn + " password=" + old_postgres_password)
                log.debug('Connected to postgres with old password')
            except psycopg2.OperationalError as exc:
                log.debug('Can\'t connect to postgres: %s', exc)
                return False
        else:
            log.debug('Can\'t connect to postgres: %s', exc)
            return False

    conn.autocommit = True
    cur = conn.cursor()
    cur.execute('SELECT pg_is_in_recovery()')
    is_in_recovery = cur.fetchone()[0]

    log.debug('is_in_recovery: %r', is_in_recovery)
    conn.close()
    return not is_in_recovery


WORKAROUNDS_QUERIES = [
    'ALTER SYSTEM RESET synchronous_standby_names',
    'SELECT pg_reload_conf()',
]

RENAME_Q_TMPL = 'ALTER DATABASE {from_db_name} RENAME TO {to_db_name}'

CHANGER_PWD_TMPL = 'ALTER ROLE {role} WITH ENCRYPTED PASSWORD \'{password}\''


def init_logging():
    root_logger = logging.getLogger()
    formatter = logging.Formatter('%(asctime)s - %(funcName)s [%(levelname)s]: %(message)s')
    root_logger.setLevel(logging.DEBUG)

    to_file = logging.FileHandler('/var/log/postgresql/restore-from-backup.log')
    to_file.setFormatter(formatter)
    to_file.setLevel(logging.DEBUG)
    root_logger.addHandler(to_file)

    to_out = logging.StreamHandler()
    to_out.setFormatter(formatter)
    to_out.setLevel(logging.WARNING)
    root_logger.addHandler(to_out)


def pg_is_dead(pg):
    cmd_args = pg.make_pg_cmd(
        'pg_ctl',
        'status',
        '--pgdata',
        pg.data,
    )
    return not run_command(cmd_args)


class RestoreActions(object):
    """
    restore action
    """

    log_restoring_seconds = 180
    sleep_while_wait_for_resting = 2.5
    sleep_while_wait_for_startup = 2.5

    def __init__(self, state_dir, rename_databases_from_to, new_postgres_password, old_postgres_password, pg):
        self.state_dir = state_dir
        self.new_postgres_password = new_postgres_password
        self.old_postgres_password = old_postgres_password
        self.rename_databases_from_to = rename_databases_from_to
        self.pg = pg
        self.restore_thread = None

    def start_postgres(self):
        """
        Start postgres and wait until it ready
        """
        self.restore_thread = threading.Thread(
            target=run_restore, kwargs=dict(pg=self.pg))

        self.restore_thread.start()
        time.sleep(self.sleep_while_wait_for_startup)
        log.debug('Restore threads started')
        log_restored_at = 0
        while True:
            if pg_is_dead(self.pg):
                self.restore_thread.join()
                raise RuntimeError('Postgresql is dead!')
            if is_postgres_ready(self.pg, self.old_postgres_password):
                log.info('Postgres ready')
                return
            if (time.time() - log_restored_at) > self.log_restoring_seconds:
                log.debug('Postgres restoring')
                log_restored_at = time.time()
            time.sleep(self.sleep_while_wait_for_resting)

    def change_postgres_password(self):
        if not self.old_postgres_password or not self.new_postgres_password:
            return
        dsn = self.pg.make_restore_dsn()
        try:
            conn = psycopg2.connect(dsn)
            # new password is ok, it's not necessary to change
            return
        except:
            pass

        username = "postgres"
        password = self.new_postgres_password
        password = "md5" + hashlib.md5('{0}{1}'.format(password, username).encode('UTF-8')).hexdigest()

        conn = psycopg2.connect(dsn + " password=" + self.old_postgres_password)
        conn.autocommit = True
        cur = conn.cursor()
        cur.execute(CHANGER_PWD_TMPL.format(role=username, password=password))

    def apply_workarounds(self):
        conn = psycopg2.connect(self.pg.make_restore_dsn())
        conn.autocommit = True
        for q in WORKAROUNDS_QUERIES:
            log.info('Apply %s', q)
            cur = conn.cursor()
            cur.execute(q)
        for from_db_name, to_db_name in self.rename_databases_from_to.items():
            log.info('Rename %s to %s', from_db_name, to_db_name)
            cur = conn.cursor()
            cur.execute(RENAME_Q_TMPL.format(from_db_name=from_db_name, to_db_name=to_db_name))
        conn.close()

    def stop_postgres(self):
        stop_restored_postgres(self.pg)
        if self.restore_thread is not None:
            self.restore_thread.join()

    def _path_to(self, name):
        return os.path.join(self.state_dir, name.__name__)

    def _state_passed(self, name):
        return os.path.exists(self._path_to(name))

    def _mark_state_passed(self, name):
        with open(self._path_to(name), 'w') as fd:
            fd.write('done at %s' % datetime.now())

    def remove_recovery_conf(self):
        """
        In case of highstate retry we will have master that fail on restart
        """
        recovery_conf = 'recovery.conf'
        if self.pg.version >= 1200:
            recovery_conf = os.path.join('conf.d', recovery_conf)
        recovery_conf = self.pg.path_to_pg_file(recovery_conf)
        recovery_signal = self.pg.path_to_pg_file('recovery.signal')
        if self.pg.version >= 1200:
            if os.path.exists(recovery_signal):
                log.info("recovery.signal %r exists. Remove it.", recovery_signal)
                os.unlink(recovery_signal)
            if os.path.exists(recovery_conf):
                log.info("recovery.conf %r exists. Erase it.", recovery_conf)
                with open(recovery_conf, 'w') as fd:
                    pass
        else:
            if os.path.exists(recovery_conf):
                log.info("recovery.conf %r exists. Remove it.", recovery_conf)
                os.unlink(recovery_conf)

    def __call__(self):
        for func in [
                self.start_postgres,
                self.change_postgres_password,
                self.apply_workarounds,
                self.stop_postgres]:
            if self._state_passed(func):
                log.info('Skip state %r, cause it completed', func)
                continue
            log.info('Start step %r', func)
            func()
            self._mark_state_passed(func)
            log.info('Step %r finished', func)
        # Always remove recovery.conf
        # If it left:
        # - next Postgre restart will fail,
        #   cause Postgre find it and will try to start point-in-time recovery to ...
        # - adding host to that cluster will fail,
        #   cause basebackup transfer it from master
        log.info("Remove recovery.conf")
        self.remove_recovery_conf()


def main():
    parser = ArgumentParser()
    parser.add_argument(
        '--recovery-state',
        metavar='PATH',
        help='path to recovery state dir',
    )
    parser.add_argument(
        '--pg-data',
        metavar='PATH',
        help='path to pg.data',
        required=True,
    )
    parser.add_argument(
        '--pg-bin',
        metavar='PATH',
        help='path to postgres utils',
        required=True,
    )
    parser.add_argument(
        '--pg-config',
        metavar='FILE',
        help='path to postgres config',
        required=True,
    )
    parser.add_argument(
        '--pg-version',
        help='PostgreSQL version',
        type=int,
        required=True,
    )
    parser.add_argument(
        '--rename-databases-from-to',
        metavar='JSON',
        help='json dict',
        required=True,
    )
    parser.add_argument(
        '--new-postgres-password',
        help='new postgres user password',
        required=False,
    )
    parser.add_argument(
        '--old-postgres-password',
        help='old postgres user password',
        required=False,
    )

    init_logging()
    args = parser.parse_args()
    rename_databases_from_to = json.loads(args.rename_databases_from_to)

    restore = RestoreActions(
        state_dir=args.recovery_state,
        new_postgres_password=args.new_postgres_password,
        old_postgres_password=args.old_postgres_password,
        rename_databases_from_to=rename_databases_from_to,
        pg=Postgres(
            pg_bin=args.pg_bin,
            pg_data=args.pg_data,
            pg_config=args.pg_config,
            pg_version=args.pg_version,
        ),
    )

    log.info('Start restore')
    try:
        restore()
    except Exception:
        log.exception('Got unexpected exception')
        raise


if __name__ == '__main__':
    main()
