#!/usr/bin/env python
# encoding: utf-8

import logging
from optparse import OptionParser
import os
import psycopg2
import sys

sys.path.append(os.path.join(os.path.dirname(__file__), '../../'))
from util.s3db import S3DB
from util.helpers import init_logging, read_config
from util.const import UPDATE_DB_CHUNKS_COUNTERS_APPLICATION_NAME


def parse_cmd_args():
    usage = """
    %prog [-d db_connstring] [-l log_level]
    """
    parser = OptionParser(usage=usage)
    parser.add_option("-l", "--log-level", dest="log_level", default="INFO",
                      help="Verbosity level (default: %default)")
    parser.add_option("-S", "--sentry-dsn", dest="sentry_dsn",
                      help="Sentry DSN, skip if don't use")
    parser.add_option("-d", "--db-connstring", dest="db_connstring", default="dbname=s3db",
                      help="Connection string to s3db database (default: %default)")
    parser.add_option("-b", "--batch-size", dest="batch_size", default="1000", type="long",
                      help="Batch size (default: %default)")
    return parser.parse_args()


def start(config):
    logging.info("Starting %s", os.path.basename(__file__))
    try:
        mydb = S3DB(config['db_connstring'], application_name=UPDATE_DB_CHUNKS_COUNTERS_APPLICATION_NAME)
    except psycopg2.OperationalError as e:
        logging.warning("Cannot connect to local server:\n%s" % str(e))
        sys.exit(1)
    if not mydb.is_master():
        logging.info("My db is replica. Nothing to do. Exiting...")
        sys.exit(0)
    batch_size = config['batch_size']
    completed = batch_size
    while completed == batch_size:
        try:
            result = mydb.update_chunks_counters(batch_size)
            mydb.commit()
            logging.info("Totally updated %s rows of %s chunks during this run",
                            result.rows, result.chunks)
            completed = result.rows
        except psycopg2.OperationalError as e:
            # continue if error is lock_timeout
            if e.pgcode == psycopg2.errorcodes.LOCK_NOT_AVAILABLE:
                logging.debug(e.pgerror)
                logging.debug("Retrying")
                mydb.rollback()
                continue

    logging.info("Stopping %s", os.path.basename(__file__))


def main():
    options, _ = parse_cmd_args()
    config = read_config(options=options)
    init_logging(config)
    start(config)


if __name__ == '__main__':
    main()
