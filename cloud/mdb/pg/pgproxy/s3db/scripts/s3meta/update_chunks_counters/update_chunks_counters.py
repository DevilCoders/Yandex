#!/usr/bin/env python
# encoding: utf-8

import logging
from optparse import OptionParser
import os
import psycopg2
import sys
from datetime import datetime
import threading

sys.path.append(os.path.join(os.path.dirname(__file__), '../../'))
from util.s3meta import S3MetaDB
from util.pgmeta import PgmetaDB
from util.helpers import init_logging, read_config
from util.const import UPDATE_META_CHUNKS_COUNTERS_APPLICATION_NAME


def parse_cmd_args():
    usage = """
    %prog [-p pgmeta_connstring] [-d db_connstring] [-u user] [-l log_level] [-S sentry_dsn]
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
    parser.add_option("-t", "--threads", dest="threads", default=1,
                      type="int", help="Threads on shard")
    return parser.parse_args()


def start(config):
    logging.info("Starting %s", os.path.basename(__file__))
    try:
        metadb_global = S3MetaDB(config['db_connstring'], application_name=UPDATE_META_CHUNKS_COUNTERS_APPLICATION_NAME)
    except psycopg2.OperationalError as e:
        logging.warning("Cannot connect to local server:\n%s" % str(e))
        sys.exit(1)
    if not metadb_global.is_master():
        logging.info("My db is replica. Nothing to do. Exiting...")
        sys.exit(0)
    metadb_global.commit()
    pgmeta = PgmetaDB(config['pgmeta_connstring'], autocommit=True, application_name=UPDATE_META_CHUNKS_COUNTERS_APPLICATION_NAME)
    not_updated = {}
    started_at = {}

    def update_shard_chunk_counters(metadb, shard_id, threads_count, thread_idx):
        started_at[shard_id][thread_idx] = datetime.now()
        try:
            db = pgmeta.get_replica('db', shard_id, user=config.get('user'), autocommit=True, application_name=UPDATE_META_CHUNKS_COUNTERS_APPLICATION_NAME)
        except psycopg2.OperationalError as e:
            logging.warning("Cannot connect to shard %s (thread %d):\n%s", shard_id, thread_idx, str(e))
            return

        chunks_meta_updated_ts = metadb.get_chunks_updated_ts_by_shard(shard_id, cid_mod_arg=threads_count,
                                                                       cid_mod_val=thread_idx)
        chunks_db_updated_ts = db.get_chunks_updated_ts(cid_mod_arg=threads_count, cid_mod_val=thread_idx)

        def update():
            meta_bids = metadb.get_all_buckets_bid()
            for (bid, cid), updated_ts in chunks_db_updated_ts.items():
                if bid not in meta_bids:
                    continue
                if (bid, cid) not in chunks_meta_updated_ts or updated_ts != chunks_meta_updated_ts[(bid, cid)]:
                    for chunk_counters in db.get_chunk_counters(bid, cid):
                        try:
                            metadb.update_chunk_counters(chunk_counters)
                        except Exception as e:
                            logging.warning('error in updating chunks:\n%s', str(e))
                            metadb.rollback()
                            raise
            metadb.commit()

        for attempt in range(1, 4):
            try:
                update()
                not_updated[shard_id].discard(thread_idx)
                break
            except psycopg2.InterfaceError:
                logging.error('error in updating counters, attempt %d, reconnect', attempt)
                metadb.reconnect()
            except Exception as e:
                logging.error('error in updating counters, attempt %d:\n%s', attempt, str(e))

        if not not_updated[shard_id]:
            metadb.set_shards_updated(shard_id, min(started_at[shard_id]),
                                      db.get_chunks_counters_queue_updated_before())
            metadb.commit()
            logging.info("Shard %s has been updated", shard_id)

    metadb_thread = {}
    for thread_idx in range(config['threads']):
        metadb_thread[thread_idx] = S3MetaDB(config['db_connstring'], application_name=UPDATE_META_CHUNKS_COUNTERS_APPLICATION_NAME)

    shards = {shard.shard_id: shard for shard in metadb_global.get_shards()}
    for shard in shards.values():
        logging.info("Shard %s last time been updated at %s", shard.shard_id, shard.last_counters_updated_ts)
        not_updated[shard.shard_id] = set(range(config['threads']))
        started_at[shard.shard_id] = [None for _ in range(config['threads'])]

    for attempt in range(1, 4):
        threads = []
        for shard, threads_set in not_updated.items():
            for thread_idx in threads_set:
                thread = threading.Thread(
                    target=update_shard_chunk_counters,
                    kwargs={'metadb': metadb_thread[thread_idx], 'shard_id': shard,
                            'threads_count': config['threads'], 'thread_idx': thread_idx}
                )
                threads.append(thread)
                thread.start()
        for thread in threads:
            thread.join()
        if not any(not_updated.values()):
            logging.info('Counters updated successfully')
            break
        logging.info('Attempt %d, not_updated: %s', attempt, not_updated)

    logging.info("Stopping %s", os.path.basename(__file__))


def main():
    options, _ = parse_cmd_args()
    config = read_config(options=options)
    init_logging(config)
    start(config)


if __name__ == '__main__':
    main()
