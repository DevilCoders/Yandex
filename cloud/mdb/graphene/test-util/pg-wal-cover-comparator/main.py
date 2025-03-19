"""
WAL coverage test data comparator
"""

import argparse
import logging
import os
import socket
import subprocess
import sys
import time
from concurrent.futures import ThreadPoolExecutor, as_completed

from scenarios import TEST_SCENARIOS


def get_waldump_out(pg_bin_dir, pg_data_dir):
    """
    Get waldump output for data dir
    """
    end = '000000010000000000000001'
    for segment in reversed(sorted(os.listdir(os.path.join(pg_data_dir, 'pg_wal')))):
        if segment.startswith('0'):
            end = segment
            break
    proc = subprocess.Popen(
        [os.path.join(pg_bin_dir, 'bin/pg_waldump'), '000000010000000000000001', end],
        cwd=pg_data_dir,
        stdout=subprocess.PIPE,
    )
    (waldump_out, _) = proc.communicate()
    return waldump_out.decode('utf-8').splitlines()


def main():
    """
    Console entry point
    """
    parser = argparse.ArgumentParser()

    parser.add_argument('--pg-bin-dir', type=str, required=True)
    parser.add_argument('--pg-data-dir', type=str, required=True)
    parser.add_argument('--storage-bin-dir', type=str, required=True)
    parser.add_argument('--threads', type=int, default=32)

    args = parser.parse_args()

    logging.basicConfig(level=logging.INFO, format='%(asctime)s %(funcName)20s %(levelname)5s: %(message)s')
    logger = logging.getLogger('main')
    with open('storage.log', 'w') as logfile:
        storage_server = subprocess.Popen(
            [os.path.join(args.storage_bin_dir, 'storage'), '--debug'],
            stderr=logfile,
            env={'TSAN_OPTIONS': 'halt_on_error=1'},
        )

    deadline = time.time() + 60
    while time.time() < deadline:
        try:
            conn = socket.create_connection(('localhost', 10011), 1)
            conn.close()
            break
        except socket.error:
            logger.info('Waiting for storage to become ready')
            time.sleep(1)

    failures = []

    waldump_out = get_waldump_out(args.pg_bin_dir, args.pg_data_dir)

    subprocess.check_call(
        [
            os.path.join(args.storage_bin_dir, 'wal_import'),
            '--wal-path',
            os.path.join(args.pg_data_dir, 'pg_wal/000000010000000000000001'),
            '--rmgr_ids',
            'all',
        ],
        env={'TSAN_OPTIONS': 'report_bugs=0'},
    )

    with ThreadPoolExecutor(max_workers=args.threads, thread_name_prefix='test-') as executor:
        future_to_fun = {}
        for scenario in TEST_SCENARIOS:
            future_to_fun[
                executor.submit(scenario, waldump_out, args.pg_data_dir, args.storage_bin_dir, logger)
            ] = scenario
        for future in as_completed(future_to_fun):
            scenario = future_to_fun[future]
            try:
                result = future.result()
            except Exception as exc:
                logger.error('%s failed with %s', scenario.__name__, exc)
                failures.append(scenario.__name__)
            else:
                logger.info('%s done: %s matched, %s skipped', scenario.__name__, result['matched'], result['skipped'])

    storage_server.terminate()

    if failures:
        logger.error('Failed scenarios: %s', ', '.join(failures))
        sys.exit(1)
