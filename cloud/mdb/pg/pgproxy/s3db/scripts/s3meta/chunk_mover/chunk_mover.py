#!/usr/bin/env python
# encoding: utf-8

import logging
from optparse import OptionParser
import os
import psycopg2
import sys
import threading

sys.path.append(os.path.join(os.path.dirname(__file__), '../../'))
from util.s3meta import S3MetaDB
from util.pgmeta import PgmetaDB
from util.helpers import init_logging, read_config
from util.const import CHUNK_MOVER_APPLICATION_NAME
import socket


def parse_cmd_args():
    usage = """
    %prog [-t threshold] [-m min_objects] [-M max_objects] [-d db_connstring] [-p pgmeta_connstring] [-u user] [-l log_level]
    """
    parser = OptionParser(usage=usage)
    parser.add_option("-l", "--log-level", dest="log_level", default="INFO",
                      help="Verbosity level (default: %default)")
    parser.add_option("-S", "--sentry-dsn", dest="sentry_dsn",
                      help="Sentry DSN, skip if don't use")
    parser.add_option("-p", "--pgmeta-connstring", dest="pgmeta_connstring", default="dbname=pgmeta",
                      help="Connection string to pgmeta database (default: %default)")
    parser.add_option("-d", "--db-connstring", dest="db_connstring", default="dbname=s3meta",
                      help="Connection string to s3db database (default: %default)")
    parser.add_option("-u", "--user", dest="user",
                      help="User which will be used for authentication in databases")
    parser.add_option("-t", "--diff-threshold", dest="diff_threshold", default="1000000",
                      help="Min objects count difference between shards (including deleted) to initiate chunk moving (default: %default)")
    parser.add_option("-M", "--max-objects", dest="max_objects", default="100000",
                      help="Max chunk size which will be moved (default: %default)")
    parser.add_option("-m", "--min-objects", dest="min_objects", default="10000",
                      help="Min chunk size which will be moved (default: %default)")
    parser.add_option("-D", "--delay", dest="delay", default="1 hour",
                      help="Dont' touch chunks younger than delay (default: %default)")
    parser.add_option("-T", "--copy-timeout", dest="copy_timeout", type="int", default="300",
                      help="Copy timeout in seconds (default: %default)")
    parser.add_option("-f", "--fail-step", dest="fail_step", type="int",
                      help="Specify fail step. Used for emulating database failure after specified commit")
    parser.add_option("-a", "--attempts", dest="attempts", default=10, type="int",
                      help="Specify number of attempts for each database to commit/rollback transactions (default: %default)")
    parser.add_option("-s", "--sleep-interval", dest="sleep_interval", default=5, type="int",
                      help="Specify sleep interval between attempts to commit/rollback on failed database (default: %default)")
    parser.add_option("-q", "--only-queue", dest="only_queue", default=False, action='store_true',
                      help="Move all chunks from queue")
    parser.add_option("--max-threads", dest="threads", default=1,
                      type="int", help="Max parallel tasks (only for --only-queue)")
    parser.add_option("--allow-same-shard", dest="allow_same_shard", default=False, action='store_true',
                      help="Allow parallel moving from/to one shard")
    return parser.parse_args()


def move(chunk_transfer, s3meta, config):
    logging.info("Found chunk ('%s', %s) for moving from %s to %s shard",
                 chunk_transfer.bid, chunk_transfer.cid,
                 chunk_transfer.source_shard, chunk_transfer.dest_shard)

    pgmeta = PgmetaDB(config['pgmeta_connstring'], autocommit=True, application_name=CHUNK_MOVER_APPLICATION_NAME)
    s3meta.commit()
    s3meta.shard_id = pgmeta.get_shard_id(socket.gethostname())

    moved = s3meta.move_chunk(
        chunk_transfer.bid, chunk_transfer.cid, chunk_transfer.source_shard,
        chunk_transfer.dest_shard, pgmeta, config.get('user'),
        config.get('attempts'), config.get('sleep_interval'),
        config.get('fail_step'),
        config.get('copy_timeout')
    )
    if not moved:
        logging.info("Chunk has not been moved. Exit.")
        exit(3)


def move_from_shard_to_shard(config, source, dest, max_count=None):
    metadb = S3MetaDB(config['db_connstring'], user=config.get('user'), application_name=CHUNK_MOVER_APPLICATION_NAME)
    moved_count = 0
    chunk_transfer = metadb.chunks_move_queue_pop(source, dest)
    if not chunk_transfer:
        logging.info("There are no task to move from %d to %d.")
    while chunk_transfer and (max_count is None or moved_count < max_count):
        move(chunk_transfer, metadb, config)
        moved_count += 1
        chunk_transfer = metadb.chunks_move_queue_pop(source, dest)


def start(config):
    logging.info("Starting %s", os.path.basename(__file__))
    try:
        metadb = S3MetaDB(config['db_connstring'], user=config.get('user'), application_name=CHUNK_MOVER_APPLICATION_NAME)
    except psycopg2.OperationalError as e:
        logging.warning("Cannot connect to local server:\n%s" % str(e))
        sys.exit(1)
    if not metadb.is_master():
        logging.info("My db is replica. Nothing to do. Exiting...")
        sys.exit(0)

    if config.get('only_queue'):
        if config.get('threads') == 1:
            chunk_transfer = metadb.chunks_move_queue_pop()
            if not chunk_transfer:
                logging.info("Chunks move queue is empty. Exiting")
                return
            while chunk_transfer:
                move(chunk_transfer, metadb, config)
                chunk_transfer = metadb.chunks_move_queue_pop()
        elif config.get('threads') > 1:
            transfers, count = metadb.get_shards_for_chunks_move(config.get('threads'), config.get('allow_same_shard'))
            threads = []
            for source, dest in transfers:
                thread = threading.Thread(
                    target=move_from_shard_to_shard,
                    kwargs={'config': config, 'source': source, 'dest': dest, 'max_count': count}
                )
                threads.append(thread)
                thread.start()
            for thread in threads:
                thread.join()
    else:
        chunk_transfer = metadb.get_chunk_to_move(
            config.get('diff_threshold'), config.get('min_objects'),
            config.get('max_objects'), config.get('delay'),
        )
        if not chunk_transfer:
            logging.info("Chunks for moving not found. Exiting")
            return
        move(chunk_transfer, metadb, config)

    logging.info("Stopping %s", os.path.basename(__file__))


def main():
    options, _ = parse_cmd_args()
    config = read_config(options=options)
    init_logging(config)
    start(config)


if __name__ == '__main__':
    main()
