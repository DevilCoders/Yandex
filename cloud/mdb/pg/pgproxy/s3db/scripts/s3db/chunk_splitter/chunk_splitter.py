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


def parse_cmd_args():
    usage = """
    %prog [-b bid [-c cid]] [-t threshold] [-m max_objects] [-p pgmeta_connstring] [-d db_connstring] [-u user] [-l log_level]
    %prog -b bid -c cid -k key [-p pgmeta_connstring] [-d db_connstring] [-u user] [-l log_level]
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
    parser.add_option("-t", "--bloat-threshold", dest="bloat_threshold", default="100000",
                      help="Split chunks, the size of which exceeds this setting (default: %default)")
    parser.add_option("-m", "--max-objects", dest="max_objects", default="80000",
                      help="Max objects which will be moved to new chunk (default: %default)")
    parser.add_option("-b", "--bid", dest="bid",
                      help="Specify bid of bucket if chunks in only specific bucket need to be splitted")
    parser.add_option("-c", "--cid", dest="cid",
                      help="Specify cid of chunk if only specific chunk need to be splitted")
    parser.add_option("-k", "--key", dest="key",
                      help="Specify key by which chunk will be splitted")
    parser.add_option("-f", "--fail-step", dest="fail_step", type="int",
                      help="Specify fail step. Used for emulate database failure after specified commit")
    parser.add_option("-a", "--attempts", dest="attempts", default=10, type="int",
                      help="Specify number of attempts for each database to commit/rollback transactions (default: %default)")
    parser.add_option("-s", "--sleep-interval", dest="sleep_interval", default=5, type="int",
                      help="Specify sleep interval between attempts to commit/rollback on failed database (default: %default)")
    return parser.parse_args()


def start(config):
    logging.info("Starting %s", os.path.basename(__file__))
    if 'cid' in config and 'bid' not in config:
        logging.fatal("bid must be specified if cid is specified")
        sys.exit(42)
    try:
        mydb = S3DB(config['db_connstring'], user=config.get('user'), application_name=const.CHUNK_SPLITTER_APPLICATION_NAME)
    except psycopg2.OperationalError as e:
        logging.warning("Cannot connect to local server:\n%s" % str(e))
        sys.exit(1)
    if not mydb.is_master():
        logging.info("My db is replica. Nothing to do. Exiting...")
        sys.exit(0)
    chunks = []
    if 'key' in config:
        if not all(key in config for key in ['bid', 'cid']):
            logging.fatal("bid and cid both must be specified in \"split by key\" mode")
            sys.exit(42)
        # If we need to split specified chunk by known key
        chunk = mydb.get_chunk(config['bid'], config['cid'])
        if not chunk:
            logging.warning("Chunk with bid = '%s' and cid = %s not found",
                            config['bid'], config['cid'])
            sys.exit(42)
        chunk.split_key = config['key']
        chunks.append(chunk)
    else:
        # Search all chunks that need to be splitted
        chunks = mydb.get_bloated_chunks(config['bloat_threshold'], bid=config.get('bid'), cid=config.get('cid'))

    # We need to commit all executed statements here because
    # connection was opened with autocommit=False and Two-Phase
    # Commit requires that connection has been cleared.
    mydb.commit()
    logging.info("Found %d chunks that need to be splitted", len(chunks))
    pgmeta = PgmetaDB(config['pgmeta_connstring'], autocommit=True, application_name=const.CHUNK_SPLITTER_APPLICATION_NAME)
    fail_step = config.get('fail_step')
    for chunk in chunks:
        chunk.split_limit = config['max_objects']
        mydb.split_chunk(chunk, pgmeta, config.get('user'), config['attempts'], config['sleep_interval'], fail_step)
    if fail_step is not None and not chunks:
        raise Exception("Nothing to split, but fail was requested")
    logging.info("Stopping %s", os.path.basename(__file__))


def main():
    options, _ = parse_cmd_args()
    config = read_config(options=options)
    init_logging(config)
    start(config)


if __name__ == '__main__':
    main()
