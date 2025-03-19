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
from util.const import UPDATE_BUCKETS_USAGE_APPLICATION_NAME


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
    parser.add_option("-t", "--threads", dest="threads", default=8, type="int", help="Threads count")
    return parser.parse_args()


def start(config):
    logging.info("Starting %s", os.path.basename(__file__))
    try:
        metadb_global = S3MetaDB(config['db_connstring'], application_name=UPDATE_BUCKETS_USAGE_APPLICATION_NAME)
    except psycopg2.OperationalError as e:
        logging.warning("Cannot connect to local server:\n%s" % str(e))
        sys.exit(1)
    if not metadb_global.is_master():
        logging.info("My db is replica. Nothing to do. Exiting...")
        sys.exit(0)
    metadb_global.commit()
    pgmeta = PgmetaDB(config['pgmeta_connstring'], autocommit=True, application_name=UPDATE_BUCKETS_USAGE_APPLICATION_NAME)
    meta_bids = metadb_global.get_all_buckets_bid()
    deleted_meta_bids = metadb_global.get_deleted_buckets_bid('12 hours')

    def update_buckets_usage(metadb, shard_id):
        logging.info('Start updating shard %s', shard_id)
        try:
            db = pgmeta.get_replica('db', shard_id, user=config.get('user'), autocommit=True, application_name=UPDATE_BUCKETS_USAGE_APPLICATION_NAME)
        except psycopg2.OperationalError as e:
            logging.warning("Cannot connect to shard %s:\n%s", shard_id, str(e))
            return

        meta_prev_hour = metadb.get_buckets_usage_prev_hour(shard_id)
        db_prev_hour = db.get_buckets_usage_prev_hour()
        for (bid, sc, end_ts) in set(meta_prev_hour.keys()) & set(db_prev_hour.keys()):
            if meta_prev_hour[(bid, sc, end_ts)] != db_prev_hour[(bid, sc, end_ts)]:
                bytesecs_diff = db_prev_hour[(bid, sc, end_ts)][0] - meta_prev_hour[(bid, sc, end_ts)][0]
                size_diff = db_prev_hour[(bid, sc, end_ts)][1] - meta_prev_hour[(bid, sc, end_ts)][1]
                metadb.correct_billing_data(shard_id, bid, sc, end_ts, bytesecs_diff, size_diff)
                logging.info("Billing data have been corrected bid={bid}, storage_class={storage_class}, "
                             "shard_id={shard_id}, end_ts={end_ts} with diff ({bytesecs_diff}, {size_diff})".format(
                    shard_id=shard_id, bid=bid, storage_class=sc, end_ts=end_ts,
                    bytesecs_diff=bytesecs_diff, size_diff=size_diff))

        buckets_usage_last_updated = metadb.get_shard_buckets_usage_last_updated(shard_id)
        for sbu in db.get_ready_buckets_usage(buckets_usage_last_updated):
            if sbu['bid'] not in meta_bids and sbu['bid'] not in deleted_meta_bids:
                continue
            logging.info("Insert bucket usage bid={bid}, storage_class={storage_class}, "
                         "shard_id={shard_id}, start_ts={start_ts}".format(shard_id=shard_id, **sbu))
            metadb.insert_shard_bucket_usage(shard_id=shard_id, **sbu)
        metadb.commit()
        logging.info('Successfully updated shard %s', shard_id)

    def update_some_shards_buckets_usage(shard_ids):
        metadb = S3MetaDB(config['db_connstring'], application_name=UPDATE_BUCKETS_USAGE_APPLICATION_NAME)
        for shard_id in shard_ids:
            update_buckets_usage(metadb, shard_id)

    threads_count = config['threads']
    thread_shards = [[] for _ in range(threads_count)]
    for i, shard in enumerate(metadb_global.get_shards()):
        thread_shards[i % threads_count].append(shard.shard_id)

    threads = []
    for shard_ids in thread_shards:
        if not shard_ids:
            continue
        thread = threading.Thread(
            target=update_some_shards_buckets_usage,
            kwargs={'shard_ids': shard_ids}
        )
        threads.append(thread)
        thread.start()
    for thread in threads:
        thread.join()
    logging.info("Stopping %s", os.path.basename(__file__))


def main():
    options, _ = parse_cmd_args()
    config = read_config(options=options)
    init_logging(config)
    start(config)


if __name__ == '__main__':
    main()
