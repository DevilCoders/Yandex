#!/usr/bin/env python3
import argparse
import json
import logging
import os
import socket
import shutil
import subprocess
import sys
import time

import psycopg2
import psycopg2.errors

WALG_CONFIG_DIR = '/etc/wal-g'
WALG_CONFIG = 'wal-g.yaml'
WALG_BIN_PATH = '/usr/bin/wal-g'
POSTGRES_CONFIG = {
    'fsync': 'off',
    'hot_standby': 'off',
    'archive_mode': 'off',
    'autovacuum': 'off',
    'ssl': 'off',
    'shared_buffers': '2GB',
    'logging_collector': 'off',
    'log_destination': 'stderr',
    'shared_preload_libraries': '',
    'log_min_duration_statement': '-1',
}


def parse_args():
    arg = argparse.ArgumentParser(description='WAL-G check backup consistency script')
    arg.add_argument(
        '-c',
        '--cluster',
        type=str,
        required=True,
        metavar='<cluster>',
        help='cluster to check backup')
    arg.add_argument(
        '-r',
        '--recover-dir',
        type=str,
        required=False,
        metavar='<dir>',
        default='/backups/recover',
        help='Folder to check backups')
    arg.add_argument(
        '-l',
        '--log-dir',
        type=str,
        required=False,
        metavar='<dir>',
        default='/var/log/walg',
        help='Folder to store PostgreSQL logs')
    return arg.parse_args()


def init_logging(level='info'):
    level = getattr(logging, level.upper())
    root = logging.getLogger()
    root.setLevel(level)
    formatter = logging.Formatter("%(levelname)s\t%(asctime)s\t\t%(message)s")
    handler = logging.StreamHandler()
    handler.setFormatter(formatter)
    handler.setLevel(level)
    root.addHandler(handler)


def walg_config_path(cluster):
    return os.path.join(WALG_CONFIG_DIR, cluster, WALG_CONFIG)


def make_walg_cmd(cluster, *args):
    return [
        WALG_BIN_PATH,
        '--config',
        walg_config_path(cluster),
        *args,
    ]


def get_free_tcp_port():
    tcp = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    tcp.bind(('', 0))
    addr, port = tcp.getsockname()
    tcp.close()
    return port


class CorruptionError(Exception):
    pass


class Postgres:
    def __init__(self, datadir, restore_command, log_file, port=None):
        self.datadir = datadir
        self.confdir = os.path.join(datadir, 'conf.d')
        self.version = self._detect_version(datadir)
        self.recovery_conf_dir = self.datadir if int(self.version) < 12 else self.confdir
        self.restore_command = restore_command
        self.port = port or get_free_tcp_port()
        self.log_file = log_file
        self._proc = None
        self.ready = False


    def _detect_version(self, path):
        with open(os.path.join(path, 'PG_VERSION'), 'r') as vfile:
            return vfile.read().strip()

    def prepare_hba(self):
        with open(os.path.join(self.confdir, 'pg_hba.conf'), 'w') as hfile:
            hfile.write('host all all 127.0.0.1/32 trust\n')
            hfile.write('host all all ::1/128 trust\n')

    def prepare_recovery_conf(self):
        with open(os.path.join(self.recovery_conf_dir, 'recovery.conf'), 'w') as rfile:
            rfile.write(f'restore_command = \'{self.restore_command}\'\n')
            rfile.write('recovery_target = \'immediate\'\n')
            rfile.write('recovery_target_action = \'promote\'\n')
        if int(self.version) < 12:
            return
        with open(os.path.join(self.datadir, 'recovery.signal'), 'w'):
            pass


    def start(self):
        cmd = [
            os.path.join('/usr/lib/postgresql', self.version, 'bin/postgres'),
            '-D',
            self.datadir,
            '-c',
            f'port={self.port}',
            '-c',
            f'data_directory={self.datadir}',
            '-c',
            'config_file={path}'.format(path=os.path.join(self.confdir, 'postgresql.conf')),
            '-c',
            'external_pid_file={path}'.format(path=os.path.join(self.datadir, 'pg.pid')),
            '-c',
            f'unix_socket_directories={self.datadir}',
            '-c',
            'hba_file={path}'.format(path=os.path.join(self.confdir, 'pg_hba.conf'))
        ]
        for key, value in POSTGRES_CONFIG.items():
            cmd += ['-c', f'{key}={value}']
        self.prepare_hba()
        self.prepare_recovery_conf()
        logging.info('Starting postgres as %s', ' '.join(cmd))
        self._proc = subprocess.Popen(cmd, stdin=subprocess.DEVNULL, stderr=self.log_file,
                                      stdout=self.log_file, close_fds=True)

    def ping(self):
        try:
            with psycopg2.connect(f'host=localhost port={self.port} user=postgres connect_timeout=1') as con:
                with con.cursor() as cur:
                    cur.execute('SELECT 1')
                    cur.fetchone()
                    return True
        except psycopg2.OperationalError:
            return False

    def check_data(self):
        databases = None
        corrupted_relations = set()
        corrupted_indexes = set()
        with psycopg2.connect(f'host=localhost port={self.port} user=postgres connect_timeout=60') as con:
            with con.cursor() as cur:
                cur.execute('SELECT datname FROM pg_database WHERE datname NOT IN (\'postgres\', \'template1\', \'template0\')')
                databases = cur.fetchall()
        for dbname, in databases:
            with psycopg2.connect(f'host=localhost port={self.port} user=postgres dbname={dbname} connect_timeout=60') as con:
                con.autocommit = True
                with con.cursor() as cur:
                    logging.info('Checking prepared xacts in %s', dbname)       
                    cur.execute('SELECT gid FROM pg_prepared_xacts WHERE database = %(dbname)s', {'dbname': dbname})
                    for gid, in cur.fetchall():
                        logging.info('Rollback %s', gid)
                        cur.execute('ROLLBACK PREPARED %(gid)s', {'gid': gid})
                    logging.info('Starting heapcheck in %s', dbname)
                    cur.execute('CREATE EXTENSION IF NOT EXISTS amcheck')
                    cur.execute('CREATE EXTENSION IF NOT EXISTS heapcheck')
                    cur.execute('SELECT oid::regclass::text FROM pg_class WHERE relkind IN (\'r\', \'t\', \'m\')')
                    for relation, in cur.fetchall():
                        try:
                            cur.execute('SELECT heap_check(%(relation)s::regclass)', {'relation': relation})
                        except psycopg2.errors.DataCorrupted as err:
                            logging.error('Corrupted relation: %s - %s', relation, err.pgerror)
                            corrupted_relations.add(relation)
                    logging.info('Starting amcheck in %s', dbname)
                    cur.execute("""
                        WITH index AS (
                            SELECT indexrelid, unnest(string_to_array(indclass::text, ' ')::oid[]) AS indclass
                                FROM pg_index WHERE indisvalid
                        )
                        SELECT DISTINCT indexrelid::regclass::text
                            FROM index i
                            JOIN pg_class c ON c.oid = i.indexrelid
                            JOIN pg_opclass o ON i.indclass = o.oid
                            JOIN pg_am a ON o.opcmethod = a.oid
                        WHERE amname = 'btree'
                            AND c.relkind in ('i')
                    """)
                    for index, in cur.fetchall():
                        try:
                            cur.execute('SELECT bt_index_parent_check(%(index)s::regclass)', {'index': index})
                        except psycopg2.errors.IndexCorrupted as err:
                            logging.error('Corrupted index: %s - %s', index, err.pgerror)
                            corrupted_indexes.add(index)
        error = ''
        if corrupted_relations:
            error += f'Corrupted relations {corrupted_relations}; '
        if corrupted_indexes:
            error += f'Corrupted indexes {corrupted_indexes}; '
        if error:
            raise CorruptionError(error)


    def dumpall_to_devnull(self):
        cmd = [
            os.path.join('/usr/lib/postgresql', self.version, 'bin/pg_dumpall'),
            '--data-only',
            '--username=postgres',
            '--host=localhost',
            f'--port={self.port}',
        ]
        logging.info('Executing %s', cmd)
        result = subprocess.run(cmd, stdin=subprocess.DEVNULL, stderr=subprocess.PIPE,
                                stdout=subprocess.DEVNULL)
        if result.returncode != 0:
            logging.info('Exit code %s; Error: %s', result.returncode, result.stderr.decode())
            raise RuntimeError('pg_dump failed')


    def wait_for_recovery(self):
        while not self.ready:
            logging.info('Waiting recovery')
            if self._proc.poll() is not None:
                logging.error(self._proc.stderr.read().decode())
                raise RuntimeError('Postgres failed')
            if self.ping():
                logging.info('Postgres ready')
                self.ready = True
                break
            time.sleep(60)

    def stop(self, timeout=600):
        if self._proc and self._proc.poll() is None:
            self._proc.terminate()
            try:
                self._proc.wait(timeout=timeout)
            except subprocess.TimeoutExpired:
                self._proc.kill()


def exec_walg_cmd(cluster, *args):
    cmd = make_walg_cmd(cluster, *args)
    logging.info('Executing %s', cmd)
    result = subprocess.run(cmd, stdin=subprocess.DEVNULL, stderr=subprocess.PIPE,
                            stdout=subprocess.PIPE)
    logging.info('Exit code %s; Result: %s', result.returncode, result.stdout.decode())
    if result.returncode != 0:
        logging.error('Error: %s', result.stderr.decode())
        raise RuntimeError('WAL-G failed')
    return result.stdout


def get_last_backup(cluster):
    try:
        backups_json = exec_walg_cmd(cluster, 'backup-list', '--json')
        if not backups_json:
            return None
        backups = json.loads(backups_json)
        if not backups:
            return None
        return backups[0]['backup_name']
    except json.decoder.JSONDecodeError:
        logging.error('Malformed backup-list result')
        return None


def get_recover_status_filepath(cluster, recoverdir):
    return os.path.join(recoverdir, '.{cluster}.status'.format(cluster=cluster))


def get_last_checked_backup(cluster, recoverdir):
    status_file_path = get_recover_status_filepath(cluster, recoverdir)
    if not os.path.exists(status_file_path):
        return None
    with open(status_file_path, 'r') as sfile:
        try:
            status = json.loads(sfile.read())
            return status['backup_name']
        except json.decoder.JSONDecodeError:
            logging.error('Malformed status file for %s', cluster)
            return None


def set_backup_status(cluster, recoverdir, backup_name, status, msg=''):
    status_file_path = get_recover_status_filepath(cluster, recoverdir)
    state = {
        'backup_name': backup_name,
        'ts': time.time(),
        'status': status,
        'message': msg,
    }
    with open(status_file_path, 'w') as sfile:
        sfile.write(json.dumps(state))


def check_last_backup(cluster, recoverdir, logdir):
    last_checked_backup = get_last_checked_backup(cluster, recoverdir)
    last_backup = get_last_backup(cluster)
    if not last_backup:
        logging.info('No backups found')
        return
    if last_checked_backup == last_backup:
        logging.info('Last backup %s already checked', last_backup)
        return
    cluster_recoverdir = os.path.join(recoverdir, cluster)
    if os.path.exists(cluster_recoverdir):
        logging.info('Found datadir from last check. Please debug errors and delete it')
        return
    postgres = None
    try:
        exec_walg_cmd(cluster, 'backup-fetch', cluster_recoverdir, 'LATEST')
        restore_command = '{walg} wal-fetch "%f" "%p" --config {config}'.format(
            walg=WALG_BIN_PATH, config=walg_config_path(cluster))
        with open(os.path.join(logdir, f'{cluster}-postgresql.log'), 'a') as logfile:
            postgres = Postgres(cluster_recoverdir, restore_command, logfile)
            postgres.start()
            postgres.wait_for_recovery()
            postgres.check_data()
            postgres.dumpall_to_devnull()
            postgres.stop()
        logging.info('Success, data is ok!')
        shutil.rmtree(cluster_recoverdir)
        set_backup_status(cluster, recoverdir, last_backup, 'OK') 
    except RuntimeError as err:
        set_backup_status(cluster, recoverdir, last_backup, 'FAILED', str(err))
        raise
    except CorruptionError as err:
        set_backup_status(cluster, recoverdir, last_backup, 'CORRUPTION', str(err))
        raise
    except Exception as err:
        logging.error(str(err))
        set_backup_status(cluster, recoverdir, last_backup, 'FAILED', 'Unknown error')
        raise
    finally:
        if postgres:
            postgres.stop()


if __name__ == '__main__':
    init_logging()
    args = parse_args()
    logging.info('Starting check consistency for %s', args.cluster)
    check_last_backup(args.cluster, args.recover_dir, args.log_dir) 
    logging.info('Finish')
