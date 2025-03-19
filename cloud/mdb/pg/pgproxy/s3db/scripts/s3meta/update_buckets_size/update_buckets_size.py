#!/usr/bin/env python
# encoding: utf-8

import logging
from optparse import OptionParser
import os
import psycopg2
import sys

sys.path.append(os.path.join(os.path.dirname(__file__), '../../'))
from util.s3meta import S3MetaDB
from util.helpers import init_logging, read_config


def parse_cmd_args():
    usage = """
    %prog [-p pgmeta_connstring] [-d db_connstring] [-u user] [-l log_level] [-S sentry_dsn]
    """
    parser = OptionParser(usage=usage)
    parser.add_option("-l", "--log-level", dest="log_level", default="INFO",
                      help="Verbosity level (default: %default)")
    parser.add_option("-S", "--sentry-dsn", dest="sentry_dsn",
                      help="Sentry DSN, skip if don't use")
    parser.add_option("-d", "--db-connstring", dest="db_connstring", default="dbname=s3meta",
                      help="Connection string to s3meta database (default: %default)")
    parser.add_option("-p", "--pgmeta-connstring", dest="pgmeta_connstring", default="dbname=pgmeta",
                      help="Connection string to pgmeta database (default: %default)")  # not used
    parser.add_option("-u", "--user", dest="user",
                      help="User which will be used for authentication in s3meta database")
    parser.add_option("-t", "--target", dest="target",
                      help="Target time")
    return parser.parse_args()


def start(config):
    logging.info("Starting %s", os.path.basename(__file__))
    try:
        metadb = S3MetaDB(config['db_connstring'])
    except psycopg2.OperationalError as e:
        logging.warning("Cannot connect to local server:\n%s" % str(e))
        sys.exit(1)
    if not metadb.is_master():
        logging.info("My db is replica. Nothing to do. Exiting...")
        sys.exit(0)

    metadb.update_buckets_size(config.get('target'))
    metadb.commit()

    logging.info("Stopping %s", os.path.basename(__file__))


def main():
    options, _ = parse_cmd_args()
    config = read_config(options=options)
    init_logging(config)
    start(config)


if __name__ == '__main__':
    main()
