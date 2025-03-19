#!/usr/bin/env python

import logging
import sys
import os
from optparse import OptionParser
import psycopg2

sys.path.append(os.path.join(os.path.dirname(__file__), '../../'))
from util.s3meta import S3MetaDB
from util.helpers import init_logging, read_config


def parse_cmd_args():
    usage = """
    %prog [-p pgmeta_connstring] [-d db_connstring] [-l log_level] [-S sentry_dsn]
    """
    parser = OptionParser(usage=usage)
    parser.add_option("-l", "--log-level", dest="log_level", default="INFO",
                      help="Verbosity level (default: %default)")
    parser.add_option("-S", "--sentry-dsn", dest="sentry_dsn",
                      help="Sentry DSN, skip if don't use")
    parser.add_option("-p", "--pgmeta-connstring", dest="pgmeta_connstring", default="dbname=pgmeta",
                      help="Connection string to pgmeta database (default: %default)")
    parser.add_option("-d", "--db-connstring", dest="db_connstring", default="dbname=s3meta",
                      help="Connection string to s3meta database (default: %default)")
    return parser.parse_args()


def start(config):
    logging.info("Starting %s", os.path.basename(__file__))

    try:
        metadb = S3MetaDB(config['db_connstring'])
    except psycopg2.OperationalError as e:
        logging.warning("Cannot connect to local server:\n%s" % str(e))
        sys.exit(1)
    if not metadb.is_master():
        logging.info("Nothing to do on replica. Exiting")
        sys.exit(0)

    try:
        metadb.refresh_bucket_stat()
        metadb.commit()
        logging.info("Buckets stats has been refreshed")
    except Exception as err:
        logging.fatal(err)
        sys.exit(1)

    logging.info("Stopping %s", os.path.basename(__file__))


def main():
    options, _ = parse_cmd_args()
    config = read_config(options=options)
    init_logging(config)
    start(config)


if __name__ == '__main__':
    main()
