"""
Helper tool for updating all worker tasks in batches
"""

import argparse
import logging
import time
from contextlib import closing

import psycopg2

GET_QUERY = """
    SELECT task_id
    FROM dbaas.worker_queue
    WHERE failed_acquire_count IS NULL
    ORDER BY create_ts
"""

UPDATE_QUERY = """
    UPDATE dbaas.worker_queue
    SET failed_acquire_count = 0
    WHERE task_id = %(task_id)s
"""


def batcher(input_list, batch_size):
    """
    Yield successive batch-sized chunks from input_list
    """
    for i in range(0, len(input_list), batch_size):
        yield input_list[i : i + batch_size]


def run_update(conn_str, batch, sleep_time):
    """
    Run updates in batches
    """
    log = logging.getLogger('updater')
    log.info('Starting update')
    with closing(psycopg2.connect(conn_str)) as conn:
        with conn as txn:
            cursor = txn.cursor()
            cursor.execute(GET_QUERY)
            tasks = [x[0] for x in cursor.fetchall()]

        log.info('Processing %s tasks', len(tasks))

        for task_batch in batcher(tasks, batch):
            log.info('Sleeping for %s seconds before batch', sleep_time)
            time.sleep(sleep_time)
            with conn as txn:
                cursor = txn.cursor()
                cursor.execute("SET synchronous_commit TO 'local'")
                for task in task_batch:
                    cursor.execute(UPDATE_QUERY, {'task_id': task})
            log.info('Updated %s rows', len(task_batch))
    log.info('Update finished')


def main():
    """
    Console entry-point
    """
    logging.basicConfig(level=logging.DEBUG, format='%(asctime)s %(levelname)s:\t%(message)s')

    parser = argparse.ArgumentParser()
    parser.add_argument('-c', '--conn', type=str, default='dbname=dbaas_metadb', help='MetaDB connection string')
    parser.add_argument('-b', '--batch', type=int, default=1000, help='Update batch size')
    parser.add_argument('-s', '--sleep', type=float, default=1, help='Time to sleep between batched (in seconds)')
    args = parser.parse_args()

    run_update(args.conn, args.batch, args.sleep)
