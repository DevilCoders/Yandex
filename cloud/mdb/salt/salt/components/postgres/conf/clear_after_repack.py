#!/usr/bin/env python

import argparse
import logging
import psycopg2
from time import time, sleep

WAITING_REPACK_SLEEP = 30 * 60  # 30 min
WAITING_REPACK_TIMEOUT = 20 * WAITING_REPACK_SLEEP    # 10 hours

logging.basicConfig(level='DEBUG', format='%(asctime)s [%(levelname)s] %(name)s:\t%(message)s')
LOG = logging.getLogger('main')


def _clear_indexes(cur, args):
    cur.execute("SELECT indexrelid, schemaname, indexrelname FROM pg_stat_all_indexes WHERE indexrelname LIKE 'index_%'")
    indexes_list = list(cur.fetchall())
    index_to_name = {indexrelid: indexrelname for (indexrelid, _, indexrelname) in indexes_list}
    index_to_schema = {indexrelid: schemaname for (indexrelid, schemaname, _) in indexes_list}
    cur.execute("SELECT indexrelid, indisvalid FROM pg_catalog.pg_index")
    index_to_valid = {indexrelid: indisvalid for (indexrelid, indisvalid) in cur.fetchall()}
    for i_id, i_name in index_to_name.items():
        try:
            source_index_id = int(i_name[6:])
        except ValueError:
            continue    # not repack index

        drop_index_cmd = "DROP INDEX CONCURRENTLY {schema}.{name}".format(schema=index_to_schema[i_id], name=i_name)
        if not index_to_valid[i_id]:  # Index not valid
            LOG.info('Clearing %s (not valid)', i_name)
            cur.execute(drop_index_cmd)
        else:   # Index is valid
            if args.all:    # Allow drop valid indexes
                if index_to_valid.get(source_index_id, False) is True:  # Source index exists and valid
                    LOG.info('Clearing %s (valid)', i_name)
                    cur.execute(drop_index_cmd)
                else:
                    LOG.info('Skip %s (is valid, source index not found)', i_name)
            else:
                LOG.info('Skip %s (is valid)', i_name)


def clear_after_repack_db(db_name, args):
    log = logging.getLogger('main')

    conn = psycopg2.connect('user=postgres dbname={dbname}'.format(dbname=db_name))
    conn.autocommit = True
    cur = conn.cursor()
    starts_at = time()
    while time() < starts_at + WAITING_REPACK_TIMEOUT:
        cur.execute("SELECT COUNT(*) FROM pg_stat_activity WHERE query LIKE 'CREATE%INDEX%'")
        if cur.fetchone()[0] == 0:
            _clear_indexes(cur, args)
            break
        else:
            log.info('Wait pg_repack')
            sleep(WAITING_REPACK_SLEEP)
    cur.close()
    conn.close()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('-a', '--all', action="store_true", default=False, help='Drop all indexes (with prefix index_)')
    args = parser.parse_args()

    conn = psycopg2.connect('user=postgres dbname=postgres')
    conn.autocommit = True
    cur = conn.cursor()
    cur.execute('SELECT pg_is_in_recovery()')
    ro = (cur.fetchone()[0] is True)
    if ro:
        LOG.info('Replica. Nothing to do here.')
        return []

    cur.execute('SELECT datname FROM pg_database')
    exclude_dbs = {'postgres', 'template0', 'template1'}
    dbs = [x[0] for x in cur.fetchall() if x[0] not in exclude_dbs]
    cur.close()
    conn.close()

    for db in dbs:
        LOG.info('Processing ' + db)
        clear_after_repack_db(db, args)


if __name__ == '__main__':
    main()
