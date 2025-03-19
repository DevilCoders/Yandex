#!/usr/bin/env python
# encoding: utf-8

import logging
from optparse import OptionParser
import os
import sys
import psycopg2
from psycopg2.extensions import ISOLATION_LEVEL_REPEATABLE_READ
from psycopg2.extras import NamedTupleCursor
from datetime import datetime
import socket

sys.path.append(os.path.join(os.path.dirname(__file__), '../../'))
from util.s3db import S3DB
from util.pgmeta import PgmetaDB
from util.helpers import init_logging, read_config
from util.const import CHECK_DB_CHUNKS_COUNTERS_APPLICATION_NAME


def parse_cmd_args():
    usage = """
    %prog [-d db_connstring] [-l log_level] [-d db_connstring]
    """
    parser = OptionParser(usage=usage)
    parser.add_option("-l", "--log-level", dest="log_level", default="INFO",
                      help="Verbosity level (default: %default)")
    parser.add_option("-S", "--sentry-dsn", dest="sentry_dsn",
                      help="Sentry DSN, skip if don't use")
    parser.add_option("-d", "--db-connstring", dest="db_connstring", default="dbname=s3db",
                      help="Connection string to s3db database (default: %default)")
    parser.add_option("-r", "--repair", dest="repair", default=False, action='store_true',
                      help="Repair counters")
    parser.add_option("--repair-billing", dest="repair_billing", default=False, action='store_true',
                      help="Repair billing too")
    parser.add_option("-c", "--limit", dest="limit", type="int", help="Chunks limit")
    parser.add_option("-o", "--offset", dest="offset", type="int", help="Chunks offset")
    parser.add_option("-b", "--bucket", dest="bucket", help="Bucket to check")
    parser.add_option("-f", "--errors-filepath", dest="errors_filepath",
                      help="File to store errors in format 'bid\tcid'")
    parser.add_option("--source-filepath", dest="source_filepath",
                      help="File in format 'bid\tcid' to check")
    parser.add_option("--storage-class", dest="storage_class", type="int",
                      help="s3.chunks_counters separate for storage_class")
    parser.add_option("--skip-objects", dest="skip_objects", default=False, action='store_true',
                      help="Don't check/repair simple_*, multipart_*, objects_parts_* keys")
    parser.add_option("--skip-deleted", dest="skip_deleted", default=False, action='store_true',
                      help="Don't check/repair deleted_* keys")
    parser.add_option("--skip-active-multipart", dest="skip_active_multipart", default=False, action='store_true',
                      help="Don't check/repair active_multipart_* keys")
    parser.add_option("--check-only-presence", dest="check_presence", default=False, action='store_true',
                      help="Check presence chunks at s3.chunks and s3.chunks_counters")
    parser.add_option("--critical-errors-filepath", dest="critical_errors_filepath",
                      help="File to store errors in format 'datetime\terror_kind\tbid\tcid'")
    parser.add_option("--run-on-replica", dest="run_on_replica", default=False, action='store_true',
                      help="Choose less priority replica, skip if master, for monitoring")
    parser.add_option("-p", "--pgmeta-connstring", dest="pgmeta_connstring", default="dbname=pgmeta",
                      help="Connection string to pgmeta database (default: %default) (Only for checking replica)")
    return parser.parse_args()


def write_error_to_files(bid, cid, errors_file=None, critical_errors_file=None, critical_error_kind=None):
    if critical_errors_file:
        critical_errors_file.write('%s\t%s\t%s\t%s\n' % (datetime.now(), critical_error_kind, bid, cid))
    if errors_file:
        errors_file.write('%s\t%s\n' % (bid, cid))


def check_chunks_counters(config, mydb, critical_errors_file):
    errors_count = 0

    repair_if_need = False
    if config['repair']:
        if config['skip_objects'] or config['skip_objects'] or config['skip_active_multipart']:
            logging.error('Cannot repair if skip some keys')
            return

        if not mydb.is_master():
            logging.info("My db is replica. Cannot repair if need")
        else:
            repair_if_need = True

    source_filepath = config.get('source_filepath')
    if source_filepath:
        bid_cid_set = set()
        with open(source_filepath, 'r') as f:
            for row in f.readlines():
                bid, cid = row.strip().split('\t')
                bid_cid_set.add((bid, cid if cid != 'None' else None))
        chunks_list = mydb.gen_chunks_from_list(bid_cid_set)
    else:
        chunks_list = mydb.gen_chunks(config.get('bucket'), config.get('limit'), config.get('offset'))

    errors_file = None
    if 'errors_filepath' in config:
        errors_file = open(config['errors_filepath'], "w")

    try:
        i = 0
        deleted_errors_bids = set()
        storage_class = config.get('storage_class')

        for i, chunk in enumerate(chunks_list):
            try:
                # Check section
                chunk = mydb.get_chunk(chunk.bid, chunk.cid)    # new transaction => update counters for this chunk
                if not chunk:
                    continue
                error_chunk = False

                target_counters = mydb.get_chunks_counters_sum(chunk, storage_class)
                if not config['skip_objects']:
                    truth_counters = mydb.get_object_counters(chunk, storage_class)
                    true_completed_parts = mydb.get_completed_parts_count(chunk, storage_class)
                    # Compare objects.parts_count with count(*) from completed_parts
                    if true_completed_parts != truth_counters['completed_parts_count']:
                        if errors_count == 0 and not error_chunk:   # first line
                            logging.info('\t'.join(['chunk', 'storage_class', 'metric', 'truth', 'target']))
                        logging.warning('\t'.join(map(str, [chunk, storage_class, 'completed_parts',
                                                            true_completed_parts, truth_counters['completed_parts_count']])))
                        error_kind = 'completed_parts' if storage_class is None else 'completed_parts_%d' % storage_class
                        write_error_to_files(chunk.bid, chunk.cid, errors_file, critical_errors_file, error_kind)
                        error_chunk = True
                    del(truth_counters['completed_parts_count'])
                else:
                    truth_counters = {}

                # Get deleted from s3.storage_delete_queue
                if not config['skip_deleted']:
                    truth_counters['deleted_objects_count'], truth_counters['deleted_objects_size'] = \
                        mydb.get_object_deleted_counters(chunk, storage_class)

                # Get active_multipart from s3.objects
                if not config['skip_active_multipart']:
                    truth_counters['active_multipart_count'] = \
                        mydb.get_object_active_multipart_count(chunk, storage_class)

                chunks_counters_queue_correction = mydb.get_chunk_counters_correction(
                    chunk, storage_class,
                    with_objects=not config['skip_objects'],
                    with_other=not (config['skip_deleted'] and config['skip_active_multipart'])
                )

                if chunks_counters_queue_correction:
                    for key, value in chunks_counters_queue_correction.items():
                        if key in truth_counters:
                            truth_counters[key] -= value

                # Compare truth and target value
                for key, truth_value in truth_counters.items():
                    target_value = target_counters[key]
                    if truth_value != target_value:
                        if key in {'deleted_objects_count', 'deleted_objects_size'}:
                            # it's ok because of splitter and mover
                            deleted_errors_bids.add(chunk.bid)
                            continue
                        if errors_count == 0 and not error_chunk:   # first line
                            logging.info('\t'.join(['chunk', 'storage_class', 'metric', 'truth', 'target']))
                        logging.warning('\t'.join(map(str, [chunk, storage_class, key, truth_value, target_counters[key]])))
                        error_kind = 'chunks_counters' if storage_class is None else 'chunks_counters_%d' % storage_class
                        write_error_to_files(chunk.bid, chunk.cid, errors_file, critical_errors_file, error_kind)
                        error_chunk = True

                if error_chunk:
                    errors_count += 1

                # Repair section
                if error_chunk and repair_if_need:
                    # if storage_class is None, we set storage_class = 0 !
                    mydb.insert_chunks_counters(chunk, truth_counters, storage_class or 0)
                    if config['repair_billing']:
                        size_change = 0
                        for key in ['simple_objects_size', 'multipart_objects_size', 'objects_parts_size']:
                            size_change += (truth_counters[key] - target_counters[key])
                        if size_change != 0:
                            mydb.insert_buckets_usage_diff(chunk, storage_class, size_change)
                    logging.info('repaired')
                mydb.commit()
            except psycopg2.OperationalError as e:
                # continue if error is lock_timeout
                if e.pgcode == psycopg2.errorcodes.LOCK_NOT_AVAILABLE:
                    logging.error('\t'.join(map(str, ['Lock timeout', chunk.bid, chunk.cid])))
                else:
                    logging.error('\t'.join(map(str, ['OperationalError', chunk.bid, chunk.cid])))
                    logging.error(e)
                mydb.rollback()
                write_error_to_files(chunk.bid, chunk.cid, errors_file)
                continue
            except Exception as e:
                logging.error('\t'.join(map(str, ['Exception', chunk.bid, chunk.cid])))
                logging.error(e)
                mydb.reconnect()
                mydb.set_session(isolation_level=ISOLATION_LEVEL_REPEATABLE_READ)
                write_error_to_files(chunk.bid, chunk.cid, errors_file)
                continue
    finally:
        logging.info('total %d, errors %d', i + 1, errors_count)
    if not config['skip_deleted']:
        for bid in deleted_errors_bids:
            # check deleted by bid, not by (bid, cid) as other counters
            count_diff, size_diff = mydb.get_deleted_diff_count_and_size(bid, storage_class)
            if count_diff != 0:
                logging.warning('Bid %s has deleted_objects_count diff = %d', bid, count_diff)
                write_error_to_files(bid, None, errors_file, critical_errors_file, 'deleted_objects_count')
            if size_diff != 0:
                logging.warning('Bid %s has deleted_objects_size diff = %d', bid, size_diff)
                write_error_to_files(bid, None, errors_file, critical_errors_file, 'deleted_objects_size')
            if repair_if_need and (count_diff != 0 or size_diff != 0):
                # if storage_class is None, we set storage_class = 0 !
                mydb.insert_fake_deleted_counters(bid, count_diff, size_diff, storage_class or 0)
                logging.info('repaired')
                mydb.commit()
    if errors_file:
        errors_file.close()


def get_excess_chunks_counters(mydb):
    cur = mydb.create_cursor(cursor_factory=NamedTupleCursor)
    cur.execute("""
        SELECT
          bid, cid
        FROM s3.chunks_counters
        WHERE (bid, cid) NOT IN (SELECT bid, cid FROM s3.chunks)
          AND (bid, cid) NOT IN (SELECT DISTINCT bid, cid FROM s3.chunks_counters_queue)
          AND (
              simple_objects_count != 0 OR
              simple_objects_size != 0 OR
              multipart_objects_count != 0 OR
              multipart_objects_size != 0 OR
              objects_parts_count != 0 OR
              objects_parts_size != 0 OR
              active_multipart_count != 0
          )
    """)
    return cur.fetchall()


def get_negative_chunks_counters(mydb):
    cur = mydb.create_cursor(cursor_factory=NamedTupleCursor)
    cur.execute("""
      SELECT
        bid, cid
      FROM s3.chunks_counters
      WHERE
          simple_objects_count < 0 OR
          simple_objects_size < 0 OR
          multipart_objects_count < 0 OR
          multipart_objects_size < 0 OR
          objects_parts_count < 0 OR
          objects_parts_size < 0 OR
          active_multipart_count < 0
        """)
    return cur.fetchall()


def check_chunks_presence(mydb, critical_errors_file):
    for row in get_excess_chunks_counters(mydb):
        logging.error('\t'.join(map(str, ['Excess', row.bid, row.cid])))
        write_error_to_files(row.bid, row.cid, None, critical_errors_file, 'excess')
    for row in get_negative_chunks_counters(mydb):
        logging.error('\t'.join(map(str, ['Negative', row.bid, row.cid])))
        write_error_to_files(row.bid, row.cid, None, critical_errors_file, 'negative')


def main():
    options, _ = parse_cmd_args()
    config = read_config(options=options)
    init_logging(config)

    logging.info("Starting %s", os.path.basename(__file__))
    mydb = S3DB(config['db_connstring'], application_name=CHECK_DB_CHUNKS_COUNTERS_APPLICATION_NAME)
    mydb.set_session(isolation_level=ISOLATION_LEVEL_REPEATABLE_READ)

    critical_filepath = config['critical_errors_filepath'] if 'critical_errors_filepath' in config else None

    if config['run_on_replica']:
        if mydb.is_master():
            logging.info("I'm master, exit.")
            return
        pgmeta = PgmetaDB(config['pgmeta_connstring'], autocommit=True, application_name=CHECK_DB_CHUNKS_COUNTERS_APPLICATION_NAME)
        if not pgmeta.replica_has_less_priority(socket.gethostname(), 'db'):
            logging.info("I'm not less priority replica, exit.")
            return
        if critical_filepath and os.path.exists(critical_filepath) and os.stat(critical_filepath).st_size != 0:
            logging.info('There is something in %s, exit' % critical_filepath)
            return

    critical_errors_file = open(config['critical_errors_filepath'], "a", buffering=0) if critical_filepath else None

    if config['check_presence']:
        check_chunks_presence(mydb, critical_errors_file)
    else:
        check_chunks_counters(config, mydb, critical_errors_file)

    if critical_errors_file:
        critical_errors_file.close()
    logging.info("Stopping %s", os.path.basename(__file__))


if __name__ == '__main__':
    main()
