#!/usr/bin/env python
# encoding: utf-8

import logging
from optparse import OptionParser
import os
import sys

sys.path.append(os.path.join(os.path.dirname(__file__), '../../'))
from util import const
from util.s3db import S3DB
from util.pgmeta import PgmetaDB
from util.helpers import init_logging, read_config
import psycopg2

MAX_MERGE_CHUNKS_COUNT = 5000


def parse_cmd_args():
    usage = """
    %prog [-b bid [-c cid]] [-p pgmeta_connstring] [-d db_connstring] [-u user] [-l log_level]
    """
    parser = OptionParser(usage=usage)
    parser.add_option("-l", "--log-level", dest="log_level", default="INFO",
                      help="Verbosity level (default: %default)")
    parser.add_option("-S", "--sentry-dsn", dest="sentry_dsn",
                      help="Sentry DSN, skip if don't use")
    parser.add_option("-p", "--pgmeta-connstring", dest="pgmeta_connstring", default="dbname=pgmeta",
                      help="Connection string to pgmeta database (default: %default)")
    parser.add_option("-d", "--db-connstring", dest="db_connstring", default="dbname=s3db",
                      help="Connection string to s3db database (default: %default)")
    parser.add_option("-u", "--user", dest="user",
                      help="User which will be used for authentication in s3meta database")
    parser.add_option("-b", "--bid", dest="bid",
                      help="Specify bid of bucket if chunks in only specific bucket need to be merged")
    parser.add_option("-c", "--cid", dest="cid",
                      help="Specify cid of chunk if only specific chunk need to be merged to previous")
    parser.add_option("-f", "--fail-step", dest="fail_step", type="int",
                      help="Specify fail step. Used for emulate database failure after specified commit")
    parser.add_option("-a", "--attempts", dest="attempts", default=10, type="int",
                      help="Specify number of attempts for each database to commit/rollback transactions (default: %default)")
    parser.add_option("-s", "--sleep-interval", dest="sleep_interval", default=5, type="int",
                      help="Specify sleep interval between attempts to commit/rollback on failed database (default: %default)")
    parser.add_option("--only-check", dest="only_check", default=False, action='store_true',
                      help="Only check amount of empty chunks")
    parser.add_option("--exclude-bids", dest="exclude_bids",
                      help="Specify comma separated bids of buckets which are not allowed to be merged")
    return parser.parse_args()


def check(mydb, config):
    could_merge_count = 0
    cannot_merge_reasons_count = {}
    for bid, cid in mydb.get_zero_chunks(bid=config.get('bid'), cid=config.get('cid')):
        could_merge, reason = mydb.get_chunk_merge_to(bid, cid, only_check=True)
        if could_merge:
            could_merge_count += 1
        else:
            if reason in cannot_merge_reasons_count:
                cannot_merge_reasons_count[reason] += 1
            else:
                cannot_merge_reasons_count[reason] = 1
    logging.info('Could merge: %d chunks', could_merge_count)
    for reason, count in cannot_merge_reasons_count.items():
        logging.info('Cannot merge: %d chunks because %s', count, reason)


def start(config):
    logging.info("Starting %s", os.path.basename(__file__))
    if 'cid' in config and 'bid' not in config:
        logging.fatal("bid must be specified if cid is specified")
        sys.exit(42)
    try:
        mydb = S3DB(config['db_connstring'], user=config.get('user'), application_name=const.CHUNK_MERGER_APPLICATION_NAME)
    except psycopg2.OperationalError as e:
        logging.warning("Cannot connect to local server:\n%s" % str(e))
        sys.exit(1)

    if config.get('only_check'):
        check(mydb, config)
        sys.exit(0)

    if not mydb.is_master():
        logging.info("My db is replica. Nothing to do. Exiting...")
        sys.exit(0)

    # Search all chunks that need to be merged
    merge_tasks = []  # list of (zero chunk, prev_chunk)
    all_chunks = set()
    exclude_bids = set(config.get('exclude_bids', '').split(','))
    for bid, cid in mydb.get_zero_chunks(bid=config.get('bid'), cid=config.get('cid')):
        if bid in exclude_bids:
            continue
        prev_chunk = mydb.get_chunk_merge_to(bid, cid)
        if prev_chunk and (bid, cid) not in all_chunks and (prev_chunk.bid, prev_chunk.cid) not in all_chunks:
            merge_tasks.append((mydb.get_chunk(bid, cid), prev_chunk))
            all_chunks.add((bid, cid))
            all_chunks.add((prev_chunk.bid, prev_chunk.cid))
        if len(merge_tasks) == MAX_MERGE_CHUNKS_COUNT:
            break

    # We need to commit all executed statements here because
    # connection was opened with autocommit=False and Two-Phase
    # Commit requires that connection has been cleared.
    mydb.commit()

    logging.info("Found %s zero chunks", len(merge_tasks) if len(merge_tasks) != MAX_MERGE_CHUNKS_COUNT else 'a lot of')
    pgmeta = PgmetaDB(config['pgmeta_connstring'], autocommit=True, application_name=const.CHUNK_MERGER_APPLICATION_NAME)
    for zero_chunk, prev_chunk in merge_tasks:
        mydb.merge_chunks(zero_chunk, prev_chunk, pgmeta, config.get('user'),
                          config['attempts'], config['sleep_interval'], config.get('fail_step'))

    # Clear zero chunks_counters without related s3.chunk and empty queue
    mydb.clear_zero_chunks_counters(bid=config.get('bid'))
    mydb.commit()
    logging.info("Stopping %s", os.path.basename(__file__))


def main():
    options, _ = parse_cmd_args()
    config = read_config(options=options)
    init_logging(config)
    start(config)


if __name__ == '__main__':
    main()
