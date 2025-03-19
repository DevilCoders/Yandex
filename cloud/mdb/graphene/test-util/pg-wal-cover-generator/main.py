"""
WAL coverage test data generator
"""

import argparse
import getpass
import logging
import os
import shutil
import subprocess
from contextlib import closing

from common.pg_cluster import PostgresCluster
from scenarios import GEN_SCENARIOS

logging.basicConfig(level=logging.INFO, format='%(asctime)s %(levelname)-8s: %(message)s')

LOG = logging.getLogger('main')


def run_initdb(bin_dir, data_dir):
    """
    Initialize postgresql database in data_dir
    """
    initdb_bin = os.path.join(bin_dir, 'bin', 'initdb')
    cmd = [initdb_bin, '-k', '--no-locale', data_dir]
    subprocess.check_call(cmd)


def prepare_config(data_dir, port):
    """
    Adjust the default postgresql.conf for WAL generation
    """
    append_lines = [
        '# Configuration added by WAL test data generator\n',
        'autovacuum = off\n',
        'archive_mode = on\n',
        'archive_command = \'/bin/false\'\n',
        'wal_log_hints = on\n',
        'wal_level = logical\n',
        'wal_compression = off\n',
        'track_commit_timestamp = on\n',
        'max_prepared_transactions = 2\n',
        f'port = {port}\n',
    ]
    file_path = os.path.join(data_dir, 'postgresql.conf')
    with open(file_path, 'a') as conf:
        conf.writelines(append_lines)


def prepare_result_dir(init_data_dir, result_data_dir):
    """
    Copy init data dir to result data dir
    """
    shutil.copytree(init_data_dir, result_data_dir)


def generate_wal(cluster):
    """
    Generate WAL test data
    """
    initial_dbs = cluster.get_dbs()
    if initial_dbs:
        raise RuntimeError('Unexpected databases in result cluster: {dbs}'.format(dbs=', '.join(initial_dbs)))

    for scenario in GEN_SCENARIOS:
        LOG.info('Starting scenario %s', scenario.__name__)
        scenario(cluster)

    with closing(cluster.get_conn('postgres')) as conn:
        conn.autocommit = True
        cursor = conn.cursor()
        cursor.execute('CHECKPOINT')


def main():
    """
    Console entry point
    """
    parser = argparse.ArgumentParser()

    parser.add_argument('--user', type=str, default=getpass.getuser())
    parser.add_argument('--bin-dir', type=str, required=True)
    parser.add_argument('--port', type=int, default=5432)
    parser.add_argument('--init-data-dir', type=str, default='data_init')
    parser.add_argument('--result-data-dir', type=str, default='data_result')

    args = parser.parse_args()

    run_initdb(args.bin_dir, args.init_data_dir)
    prepare_config(args.init_data_dir, args.port)
    prepare_result_dir(args.init_data_dir, args.result_data_dir)
    result_cluster = PostgresCluster(args.user, args.result_data_dir, args.bin_dir, args.port)

    generate_wal(result_cluster)
