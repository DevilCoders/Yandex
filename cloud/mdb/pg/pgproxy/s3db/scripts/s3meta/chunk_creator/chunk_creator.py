#!/usr/bin/env python
# encoding: utf-8

import logging
from optparse import OptionParser
import os
import psycopg2
import sys

sys.path.append(os.path.join(os.path.dirname(__file__), '../../'))
from util.s3meta import S3MetaDB
from util.pgmeta import PgmetaDB
from util.helpers import complete_tpc, init_logging, read_config


def parse_cmd_args():
    usage = """
    %prog [-p pgmeta_connstring] [-d db_connstring] [-u user] [-l log_level]
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
    parser.add_option("-u", "--user", dest="user",
                      help="User which will be used for authentication in s3db database")
    parser.add_option("-f", "--fail-step", dest="fail_step", type="int",
                      help="Specify fail step. Used for emulate database failure after specified commit")
    parser.add_option("-a", "--attempts", dest="attempts", default=10, type="int",
                      help="Specify number of attempts for each database to commit/rollback transactions (default: %default)")
    parser.add_option("-s", "--sleep-interval", dest="sleep_interval", default=5, type="int",
                      help="Specify sleep interval between attempts to commit/rollback on failed database (default: %default)")
    return parser.parse_args()


def start(config):
    logging.info("Starting %s", os.path.basename(__file__))
    fail_step = config.get('fail_step')
    try:
        mydb = S3MetaDB(config['db_connstring'])
    except psycopg2.OperationalError as e:
        logging.warning("Cannot connect to local server:\n%s" % str(e))
        sys.exit(1)
    if not mydb.is_master():
        logging.info("My db is replica. Nothing to do. Exiting...")
        sys.exit(0)
    mydb.commit()
    pgmeta = PgmetaDB(config['pgmeta_connstring'], autocommit=True)
    tpc_prefix = "create"
    while True:
        mydb.begin_tpc(tpc_prefix)
        chunk = mydb.chunk_queue_pop()
        if not chunk:
            logging.info("Queue is empty. Nothing to do")
            break
        logging.info("Found new chunk ('%s', %s) on shard %s",
                     chunk.bid, chunk.cid, chunk.shard_id)

        db = pgmeta.get_master('db', chunk.shard_id, user=config.get('user'))
        db_chunk = db.get_chunk(chunk.bid, chunk.cid)
        db.commit()

        metadb, bucket = pgmeta.get_s3meta_rw_by_bucket(chunk.bid, user=config.get('user'))
        mydb.shard_id = metadb.shard_id

        if db_chunk:
            logging.info("Chunk ('%s', %s) already exists on %s shard s3db. Deleting from queue",
                         chunk.bid, chunk.cid, chunk.shard_id)
            complete_tpc([mydb], pgmeta, config['attempts'],
                         config['sleep_interval'], fail_step)
            continue
        # chunk doesn't exists on db we need to move chunk to db
        logging.info("Chunk ('%s', %s) doesn't exists on %s shard s3db. Creating",
                     chunk.bid, chunk.cid, chunk.shard_id)
        db.begin_tpc("{0}_{1}_{2}".format(tpc_prefix, chunk.bid, chunk.cid))
        db.add_chunk(chunk)
        db.shard_id = metadb.get_chunk(chunk.bid, chunk.cid).shard_id
        complete_tpc(
            [db, mydb], pgmeta, config['attempts'],
            config['sleep_interval'], fail_step
        )
    logging.info("Stopping %s", os.path.basename(__file__))


def main():
    options, _ = parse_cmd_args()
    config = read_config(options=options)
    init_logging(config)
    start(config)


if __name__ == '__main__':
    main()
