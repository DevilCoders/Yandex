#!/usr/bin/env python
# encoding: utf-8

import logging
from optparse import OptionParser
import os
import sys

sys.path.append(os.path.join(os.path.dirname(__file__), '../../'))
from util.s3meta import S3MetaDB
from util.pgmeta import PgmetaDB
from util.helpers import init_logging, read_config


def parse_cmd_args():
    usage = """
    %prog [-p pgmeta_connstring] [-d db_connstring] [-u user] [-l log_level] [-t target_time]
    """
    parser = OptionParser(usage=usage)
    parser.add_option("-l", "--log-level", dest="log_level", default="INFO",
                      help="Verbosity level (default: %default)")
    parser.add_option("-p", "--pgmeta-connstring", dest="pgmeta_connstring", default="dbname=pgmeta",
                      help="Connection string to pgmeta database (default: %default)")
    parser.add_option("-d", "--db-connstring", dest="db_connstring", default="dbname=s3meta",
                      help="Connection string to s3meta database (default: %default)")
    parser.add_option("-u", "--user", dest="user",
                      help="User which will be used for authentication in s3meta database")
    parser.add_option("-t", "--target", dest="target",
                      help="Target time")
    return parser.parse_args()


def start(config):
    logging.info("Starting %s", os.path.basename(__file__))
    metadb = S3MetaDB(config['db_connstring'])
    if not metadb.is_master():
        logging.info("My db is replica. Nothing to do. Exiting...")
        sys.exit(0)
    metadb.commit()
    pgmeta = PgmetaDB(config['pgmeta_connstring'], autocommit=True)
    target_ts = config.get('target')

    meta_bids = metadb.get_all_buckets_bid()
    deleted_meta_bids = metadb.get_deleted_buckets_bid('12 hours')
    meta_to_clear = {(row['shard_id'], row['bid'], row['storage_class']) for row in metadb.get_buckets_size(target_ts)}
    meta_to_clear = meta_to_clear.union((row['shard_id'], row['bid'], 0) for row in metadb.get_unused_buckets())

    for shard in metadb.get_shards():
        logging.info("Shard %s last time been updated at %s", shard.shard_id, shard.last_counters_updated_ts)
        db = pgmeta.get_replica('db', shard.shard_id, user=config.get('user'), autocommit=True)

        for sbs in db.get_buckets_size(target_ts):
            if sbs['bid'] not in meta_bids:
                continue
            logging.info("Insert bucket size bid={bid}, storage_class={storage_class}, shard_id={shard_id}, size={size}".format(
                shard_id=shard.shard_id, **sbs))
            metadb.insert_shard_bucket_size(shard_id=shard.shard_id, target_ts=target_ts, **sbs)
            meta_to_clear.discard((shard.shard_id, sbs['bid'], sbs['storage_class']))

        # Because MDB-10835
        buckets_usage_last_updated = metadb.get_shard_buckets_usage_last_updated(shard.shard_id, target_ts)
        for sbu in db.get_ready_buckets_usage(buckets_usage_last_updated, ignore_queue=True):
            if sbu['bid'] not in meta_bids and sbu['bid'] not in deleted_meta_bids:
                continue
            logging.info("Insert bucket usage bid={bid}, storage_class={storage_class}, "
                         "shard_id={shard_id}, start_ts={start_ts}".format(shard_id=shard.shard_id, **sbu))
            metadb.insert_shard_bucket_usage(shard_id=shard.shard_id, **sbu)

        metadb.commit()
        logging.info("Shard %s has been updated", shard.shard_id)

    for shard_id, bid, storage_class in meta_to_clear:
        metadb.insert_shard_bucket_size(shard_id=shard_id, bid=bid, storage_class=storage_class, target_ts=target_ts, size=0)
    metadb.commit()

    logging.info("Stopping %s", os.path.basename(__file__))


def main():
    options, _ = parse_cmd_args()
    config = read_config(options=options)
    init_logging(config)
    start(config)


if __name__ == '__main__':
    main()
