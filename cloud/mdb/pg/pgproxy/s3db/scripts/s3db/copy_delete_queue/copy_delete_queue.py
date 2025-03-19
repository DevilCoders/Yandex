#!/usr/bin/env python
# encoding: utf-8
import logging
import time
from optparse import OptionParser
import os
import psycopg2
import sys

sys.path.append(os.path.join(os.path.dirname(__file__), '../../'))
from util.s3db import S3DB
from util.helpers import init_logging, read_config


GLACIER_RUN_FILE_FLAG = '/tmp/.glacier_run_file.flag'
GLACIER_STORAGE_CLASS = 2

# for arcadia tests
if 'YA_TEST_CONTEXT_FILE' in os.environ:
    import yatest.common as yc
    GLACIER_RUN_FILE_FLAG = yc.runtime.work_path('.glacier_run_file.flag')


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
    parser.add_option("-t", "--min-free-time", dest="min_free_time", default="1 year",
                      help="Minimum free store time interval for objects (default: %default)")
    return parser.parse_args()


def main():
    options, _ = parse_cmd_args()
    config = read_config(options=options)
    init_logging(config)

    logging.info("Starting %s", os.path.basename(__file__))

    try:
        mydb = S3DB(config['db_connstring'])
    except psycopg2.OperationalError as e:
        logging.warning("Cannot connect to local server:\n%s", str(e))
        sys.exit(1)

    if not mydb.is_master():
        logging.info("My db is replica. Nothing to do. Exiting...")
        sys.exit(0)

    last_ts = 0.0
    if os.path.exists(GLACIER_RUN_FILE_FLAG):
        last_ts = os.path.getmtime(GLACIER_RUN_FILE_FLAG)

    free_time = config['min_free_time']
    mtime = time.time()

    try:
        mydb.copy_to_billing_delete_queue(free_time, last_ts=last_ts, storage_class=GLACIER_STORAGE_CLASS)
        mydb.commit()
        logging.info("Copied all new rows from s3.storage_delete_queue to s3.billing_delete_queue.")

    except psycopg2.OperationalError:
        logging.exception('Cannot copy rows from s3.storage_delete_queue to s3.billing_delete_queue.')
        raise

    if os.path.exists(GLACIER_RUN_FILE_FLAG):
        atime = os.path.getatime(GLACIER_RUN_FILE_FLAG)
        os.utime(GLACIER_RUN_FILE_FLAG, (atime, mtime))
    else:
        with open(GLACIER_RUN_FILE_FLAG, 'w'):
            pass

    logging.info("Stopping %s", os.path.basename(__file__))


if __name__ == '__main__':
    main()
