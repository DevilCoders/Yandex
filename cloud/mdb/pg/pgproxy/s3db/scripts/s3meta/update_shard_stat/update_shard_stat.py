#!/usr/bin/env python

import logging
import sys
import os

from optparse import OptionParser

sys.path.append(os.path.join(os.path.dirname(__file__), '../../'))
from util.s3meta import S3MetaDB
from util.helpers import init_logging, read_config


def parse_cmd_args():
    usage = """
    %prog [-d db_connstring] [-l log_level]
    """
    parser = OptionParser(usage=usage)
    parser.add_option("-l", "--log-level", dest="log_level", default="INFO",
                      help="Verbosity level (default: %default)")
    parser.add_option("-d", "--db-connstring", dest="db_connstring", default="dbname=s3meta",
                      help="Connection string to s3meta database (default: %default)")
    return parser.parse_args()


def start(config):
    logging.info("Starting %s", os.path.basename(__file__))
    try:
        mydb = S3MetaDB(config['db_connstring'])
        if not mydb.is_master():
            logging.info("Nothing to do on replica. Exiting")
            sys.exit(0)
        mydb.refresh_shard_stat()
        mydb.commit()
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
