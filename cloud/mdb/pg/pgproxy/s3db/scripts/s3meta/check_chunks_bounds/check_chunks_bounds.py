#!/usr/bin/env python
# encoding: utf-8

import logging
from optparse import OptionParser
import os
import sys
from time import sleep
from datetime import datetime
from collections import defaultdict
import psycopg2
from psycopg2.extras import NamedTupleCursor
import socket

sys.path.append(os.path.join(os.path.dirname(__file__), '../../'))
from util.pgmeta import PgmetaDB
from util.s3meta import S3MetaDB
from util.helpers import init_logging, read_config
from util.chunk import NEW_COUNTERS_KEYS
from util.const import CHECK_META_CHUNKS_COUNTERS_APPLICATION_NAME

RETRY_COUNT = 3
SLEEP_SEC = 3


def parse_cmd_args():
    usage = """
    %prog [-d db_connstring] [-p pgmeta_connstring] [-l log_level]
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
    parser.add_option("--critical-errors-filepath", dest="errors_filepath",
                      help="File to store errors in format 'datetime\terror_kind\tbid\tcid_or_storage_class'")
    parser.add_option("--run-on-replica", dest="run_on_replica", default=False, action='store_true',
                      help="Choose less priority replica, skip if master, for monitoring")
    parser.add_option("--period", dest="period", default="30 days",
                      help="Dont' check records younger than period ago (default: %default); allowed 'infinity'")
    parser.add_option("--skip-equal-buckets-usage", dest="skip_equal_buckets_usage", default=False, action='store_true',
                      help="Skip checking equivalence buckets_usage in meta and db (for tests)")
    return parser.parse_args()


def write_error_to_file(bid, cid_or_storage_class, errors_file=None, error_kind=None):
    if errors_file:
        errors_file.write('%s\t%s\t%s\t%s\n' % (datetime.now(), error_kind, bid, cid_or_storage_class))


def retry(func):
    def wrapped(db, *args, **kwargs):
        attempt = 0
        while attempt < RETRY_COUNT:
            try:
                return func(db, *args, **kwargs)
            except (psycopg2.InterfaceError, psycopg2.errors.AdminShutdown, psycopg2.errors.TooManyConnections) as e:
                logging.warning('Retrying %s on shard %s, attempt %s', repr(e).replace('\n', ' '), db.shard_id, attempt + 1)
                sleep(SLEEP_SEC)
                db.reconnect()
                attempt += 1
        return func(db, *args, **kwargs)
    return wrapped


@retry
def get_metadb_counts(metadb):
    cur = metadb.create_cursor(cursor_factory=NamedTupleCursor)
    cur.execute("""
        SELECT bid, shard_id, count(*) AS count FROM s3.chunks GROUP BY (bid, shard_id)
    """)
    return {(row.bid, row.shard_id): row.count for row in cur.fetchall()}


@retry
def get_db_counts(db, shard_id):
    cur = db.create_cursor(cursor_factory=NamedTupleCursor)
    cur.execute("""
        SELECT bid, count(*) AS count FROM s3.chunks GROUP BY bid
    """)
    return {(row.bid, shard_id): row[1] for row in cur.fetchall()}


@retry
def get_db_not_empty_buckets_size(db):
    cur = db.create_cursor()
    cur.execute("""
      SELECT
          bid,
          storage_class,
          sum(simple_objects_size) + sum(multipart_objects_size) + sum(objects_parts_size)
        FROM s3.chunks_counters
        WHERE simple_objects_size + multipart_objects_size + objects_parts_size > 0
      GROUP BY bid, storage_class
    """)
    db.log_last_query(cur)
    return cur.fetchall()


@retry
def get_cids(db, bid, shard_id=None):
    cur = db.create_cursor()
    if shard_id is not None:    # check on db
        cur.execute("""
            SELECT cid FROM s3.chunks WHERE bid = %(bid)s and shard_id = %(shard_id)s
        """, {'bid': bid, 'shard_id': shard_id})
    else:   # check on meta
        cur.execute("""
            SELECT cid FROM s3.chunks WHERE bid = %(bid)s
        """, {'bid': bid})
    return {row[0] for row in cur.fetchall()}


@retry
def meta_get_chunks_to_creation(metadb):
    cur = metadb.create_cursor()
    cur.execute("""
        SELECT bid, cid FROM s3.chunks_create_queue
    """)
    return {(row[0], row[1]) for row in cur.fetchall()}


@retry
def meta_get_chunks_to_deletion(metadb):
    cur = metadb.create_cursor()
    cur.execute("""
        SELECT bid, cid FROM s3.chunks_delete_queue
    """)
    return {(row[0], row[1]) for row in cur.fetchall()}


def check_chunks_bounds(metadb, s3db_map, errors_file=None):
    db_counts = {}
    for shard_id, s3db in s3db_map.items():
        db_counts.update(get_db_counts(s3db, shard_id))
    metadb_counts = get_metadb_counts(metadb)
    meta_bids = metadb.get_all_buckets_bid()
    meta_chunks_to_creation = meta_get_chunks_to_creation(metadb)
    meta_chunks_to_deletion = meta_get_chunks_to_deletion(metadb)

    for bid, shard_id in set(metadb_counts.keys()).union(set(db_counts.keys())):
        if bid == '00000000-0000-0000-0000-000000000000':
            continue
        if bid not in meta_bids:
            continue
        in_meta = metadb_counts.get((bid, shard_id), 0)
        in_db = db_counts.get((bid, shard_id), 0)
        if in_meta == in_db:
            continue
        db_cids = get_cids(s3db_map[shard_id], bid)
        meta_cids = get_cids(metadb, bid, shard_id)
        meta_not_db = meta_cids - db_cids
        db_not_meta = db_cids - meta_cids
        if meta_not_db:
            logging.warning('Found in meta, not in db:  bid %s, cids: %s', bid, ','.join(map(str, meta_cids - db_cids)))
            if errors_file:
                for cid in meta_cids - db_cids:
                    if (bid, cid) not in meta_chunks_to_creation:
                        write_error_to_file(bid, cid, errors_file, 'db_chunk_missed')

        if db_not_meta:
            logging.warning('Found in %d db shard, not in meta: bid %s, cids: %s', shard_id, bid, ','.join(map(str, db_cids - meta_cids)))
            if errors_file:
                for cid in db_cids - meta_cids:
                    if (bid, cid) not in meta_chunks_to_deletion:
                        write_error_to_file(bid, cid, errors_file, 'meta_chunk_missed')


@retry
def db_old_object_presents(db, bid, storage_class):
    cur = db.create_cursor()
    cur.execute("""
        SELECT 1
        FROM s3.objects
        WHERE bid=%(bid)s AND storage_class=coalesce(%(storage_class)s, 0)
          AND created < (current_timestamp - interval '70 minute')
          AND data_size > 0
        LIMIT 1
    """, {'bid': bid, 'storage_class': storage_class})
    row = cur.fetchone()
    if row:
        return row[0]


@retry
def meta_get_billing_presence_hour_ago(metadb):
    cur = metadb.create_cursor(cursor_factory=NamedTupleCursor)
    cur.execute("""
      SELECT bid, shard_id, storage_class, size
        FROM s3.get_buckets_size_at_time(current_timestamp - interval '70 minute')
        WHERE size > 0
    """)
    return {(row.bid, row.shard_id, row.storage_class) for row in cur.fetchall()}


def check_billing_buckets_presence(metadb, s3db_map, errors_file=None):
    meta_data = defaultdict(dict)
    meta_bids = metadb.get_all_buckets_bid()
    meta_old_billing_presence = meta_get_billing_presence_hour_ago(metadb)
    for row in metadb.get_buckets_size():
        if row['bid'] in meta_bids and row['size'] > 0:
            meta_data[row['shard_id']][(row['bid'], row['storage_class'])] = row['size']
    for shard_id, db in s3db_map.items():
        db_data = {}
        for bid, storage_class, size in get_db_not_empty_buckets_size(db):
            if bid in meta_bids:
                db_data[(bid, storage_class)] = size
        meta_keys = set(meta_data[shard_id].keys())
        db_keys = set(db_data.keys())
        for (bid, storage_class) in meta_keys - db_keys:
            if not db_old_object_presents(db, bid, storage_class):
                # all objects in bid with storage_class have been deleted last hour
                continue
            logging.warning('Presents in meta, not in %s shard: bid %s, storage_class %s, size %s',
                            shard_id, bid, storage_class, meta_data[shard_id][(bid, storage_class)])
            if errors_file:
                write_error_to_file(bid, storage_class, errors_file, 'unknown_db_chunks_counters')
        for (bid, storage_class) in db_keys - meta_keys:
            if (bid, shard_id, storage_class) not in meta_old_billing_presence:
                # bucket has been moved last hour
                continue
            if not db_old_object_presents(db, bid, storage_class):
                # bucket has been created last hour
                continue
            logging.warning('Presents in %s shard, not in meta: bid %s, storage_class %s, size %s',
                            shard_id, bid, storage_class, db_data[(bid, storage_class)])
            if errors_file:
                write_error_to_file(bid, storage_class, errors_file, 'missed_meta_billing_data')


@retry
def _meta_get_buckets_usage_set(metadb, shard_id, period):
    cur = metadb.create_cursor(cursor_factory=NamedTupleCursor)
    cur.execute("""
        SELECT bid, storage_class, start_ts, size_change, byte_secs
        FROM s3.buckets_usage
        WHERE shard_id = %(shard_id)s
          AND (%(period)s IS NULL OR start_ts >= (current_timestamp - %(period)s::interval))
          AND end_ts < (current_timestamp - interval '70 minute')
    """, {'shard_id': shard_id, 'period': period})
    metadb.log_last_query(cur)
    return {(row.bid, row.storage_class, row.start_ts, row.size_change, row.byte_secs) for row in cur.fetchall()}


@retry
def _db_get_buckets_usage_set(db, shard_id, period):
    cur = db.create_cursor(cursor_factory=NamedTupleCursor)
    cur.execute("""
                SELECT bid, storage_class, start_ts, size_change, byte_secs
                FROM s3.buckets_usage
                WHERE (%(period)s IS NULL OR start_ts >= (current_timestamp - %(period)s::interval))
                  AND end_ts < (current_timestamp - interval '70 minute')
            """, {'shard_id': shard_id, 'period': period})
    db.log_last_query(cur)
    return {(row.bid, row.storage_class, row.start_ts, row.size_change, row.byte_secs) for row in cur.fetchall()}


@retry
def _meta_get_buckets_usage_presence(metadb, shard_id, storage_class, start_ts, size_change, byte_secs):
    cur = metadb.create_cursor(cursor_factory=NamedTupleCursor)
    cur.execute("""
        SELECT 1
        FROM s3.buckets_usage
        WHERE start_ts = %(start_ts)s AND shard_id = %(shard_id)s AND storage_class = %(storage_class)s
          AND size_change = %(size_change)s AND byte_secs = %(byte_secs)s
    """, {'shard_id': shard_id, 'storage_class': storage_class, 'start_ts': start_ts,
          'size_change': size_change, 'byte_secs': byte_secs})
    return cur.fetchone()


@retry
def _db_get_buckets_usage_presence(db, storage_class, start_ts, size_change, byte_secs):
    cur = db.create_cursor(cursor_factory=NamedTupleCursor)
    cur.execute("""
        SELECT 1
        FROM s3.buckets_usage
        WHERE start_ts = %(start_ts)s AND storage_class = %(storage_class)s
          AND size_change = %(size_change)s AND byte_secs = %(byte_secs)s
    """, {'storage_class': storage_class, 'start_ts': start_ts,
          'size_change': size_change, 'byte_secs': byte_secs})
    return cur.fetchone()


def check_equal_buckets_usage_in_meta_and_db(metadb, s3db_map, period, errors_file=None):
    meta_bids = metadb.get_all_buckets_bid()
    for shard_id, db in s3db_map.items():
        meta_set = _meta_get_buckets_usage_set(metadb, shard_id, period)
        db_set = _db_get_buckets_usage_set(db, shard_id, period)

        for (bid, storage_class, start_ts, size_change, byte_secs) in meta_set - db_set:
            if _db_get_buckets_usage_presence(db, storage_class, start_ts, size_change, byte_secs):
                continue
            logging.warning('Presents in meta, not in %s shard: bid %s, storage_class %s, start_ts %s, size_change %s, byte_secs %s',
                            shard_id, bid, storage_class, start_ts, size_change, byte_secs)
            if errors_file:
                write_error_to_file(bid, storage_class, errors_file, 'missed_db_buckets_usage')
        for (bid, storage_class, start_ts, size_change, byte_secs) in db_set - meta_set:
            if bid not in meta_bids:
                continue
            if _meta_get_buckets_usage_presence(metadb, shard_id, storage_class, start_ts, size_change, byte_secs):
                continue
            logging.warning('Presents in %s shard, not in meta: bid %s, storage_class %s, start_ts %s, size_change %s, byte_secs %s',
                            shard_id, bid, storage_class, start_ts, size_change, byte_secs)
            if errors_file:
                write_error_to_file(bid, storage_class, errors_file, 'missed_meta_buckets_usage')


def get_meta_all_billing_buckets_bid(metadb):
    cur = metadb.create_cursor()
    cur.execute("""
        SELECT bid, storage_class, shard_id
        FROM (
          SELECT bid, storage_class, shard_id FROM s3.buckets_size
          UNION
          SELECT bid, storage_class, shard_id FROM s3.buckets_usage
        ) b
    """)
    metadb.log_last_query(cur)
    return cur.fetchall()


@retry
def _check_meta_bucket_sizes(metadb, bid, storage_class, shard_id, period, errors_file=None):
    cur = metadb.create_cursor(cursor_factory=NamedTupleCursor)
    cur.execute("""
        SELECT target_ts, size
        FROM s3.buckets_size
        WHERE bid = %(bid)s
          AND storage_class = %(storage_class)s
          AND shard_id = %(shard_id)s
          AND (%(period)s IS NULL OR target_ts >= (current_timestamp - %(period)s::interval))
    """, {'bid': bid, 'storage_class': storage_class, 'shard_id': shard_id, 'period': period}
    )
    metadb.log_last_query(cur)
    sizes = [(x.target_ts, x.size, 'size') for x in cur.fetchall()]
    buckets_usage_from = min([s[0] for s in sizes]) if sizes else None     # get first buckets_size target_ts

    cur = metadb.create_cursor(cursor_factory=NamedTupleCursor)
    cur.execute("""
        SELECT end_ts, size_change
        FROM s3.buckets_usage
        WHERE bid= %(bid)s
          AND storage_class = %(storage_class)s
          AND shard_id = %(shard_id)s
          AND (%(buckets_usage_from)s IS NULL OR start_ts >= %(buckets_usage_from)s)
    """, {'bid': bid, 'storage_class': storage_class, 'shard_id': shard_id, 'buckets_usage_from': buckets_usage_from}
    )
    metadb.log_last_query(cur)
    usages = [(x.end_ts, x.size_change, 'usage') for x in cur.fetchall()]

    # sizes: [target_ts, size at target_ts, 'size']
    # usages: [change_ts, size_change at change_ts, 'usage']
    # merge sizes and usages and sort by (timestamp, [usage, size]), so
    # if size and usage have the same timestamp, at first to common list we add usage
    timeline = sorted(sizes + usages, key=lambda x: str(x[0])+('0' if x[2] == 'usage' else '1'))

    if not timeline:
        return

    # if first record type is not size => size = 0 (bucket added after last buckets_size update)
    if timeline[0][2] == 'usage':
        current_size = 0
    else:
        current_size = timeline[0][1]

    for ts, size, kind in timeline:
        if kind == 'size':
            if size != current_size:
                logging.warning('Shard %s, bid %s, storage_class %s, ts= %s, true size = %s, usage size = %s',
                                shard_id, bid, storage_class, ts, size, current_size)
                if errors_file:
                    write_error_to_file(bid, storage_class, errors_file, 'billing_size')
                current_size = size
        else:   # usage
            current_size += size


def check_billing_timeline(metadb, period, errors_file=None):
    meta_billing_bid_sc = get_meta_all_billing_buckets_bid(metadb)
    for bid, storage_class, shard_id in meta_billing_bid_sc:
        _check_meta_bucket_sizes(metadb, bid, storage_class, shard_id, period, errors_file)


@retry
def db_has_recent_bucket_usage(db, bid, storage_class):
    cur = db.create_cursor()
    cur.execute("""
        SELECT 1
        FROM s3.buckets_usage
        WHERE bid=%(bid)s AND storage_class=coalesce(%(storage_class)s, 0)
          AND end_ts > (current_timestamp - interval '70 minute')
        LIMIT 1
    """, {'bid': bid, 'storage_class': storage_class})
    row = cur.fetchone()
    if row:
        return row[0]


@retry
def _meta_get_deleted_buckets_size(metadb, period):
    cur = metadb.create_cursor(cursor_factory=NamedTupleCursor)
    cur.execute("""
          SELECT bid, storage_class, shard_id, sum(size) AS size
          FROM s3.get_buckets_size_at_time()
          WHERE size != 0
            AND bid IN (
              -- select buckets deleted more than 2 hours ago
              SELECT bid
              FROM s3.buckets_history
              WHERE deleted < (current_timestamp - interval '120 minute')
                AND (%(period)s IS NULL OR deleted >= (current_timestamp - %(period)s::interval))
            )
          GROUP BY bid, storage_class, shard_id
        """, {'period': period})
    return cur.fetchall()


def check_deleted_buckets(metadb, s3db_map, period, errors_file=None):
    for row in _meta_get_deleted_buckets_size(metadb, period):
        if not db_has_recent_bucket_usage(s3db_map[row.shard_id], row.bid, row.storage_class):
            logging.warning('Deleted bucket %s has billing size = %s', row.bid, row.size)
            if errors_file:
                write_error_to_file(row.bid, row.storage_class, errors_file, 'deleted_bucket')


def check_negative_bucket_size(metadb, errors_file=None):
    cur = metadb.create_cursor(cursor_factory=NamedTupleCursor)
    cur.execute("""
      SELECT bid, storage_class, size
      FROM s3.buckets_size
      WHERE size < 0
    """)
    for row in cur.fetchall():
        logging.warning('Bucket %s has negative billing size = %s', row.bid, row.size)
        if errors_file:
            write_error_to_file(row.bid, row.storage_class, errors_file, 'negative_size')


@retry
def _meta_get_shard_counters(metadb, shard_id):
    cur = metadb.create_cursor(cursor_factory=NamedTupleCursor)
    cur.execute("""
                SELECT *
                FROM s3.chunks_counters JOIN s3.chunks USING (bid, cid)
                WHERE shard_id = %(shard_id)s
                  AND (simple_objects_size != 0 OR multipart_objects_size != 0 OR objects_parts_size != 0)
            """, {'shard_id': shard_id})
    metadb.log_last_query(cur)
    return {(row.bid, row.cid, row.storage_class): row for row in cur.fetchall()}


@retry
def _db_get_counters(db):
    cur = db.create_cursor(cursor_factory=NamedTupleCursor)
    cur.execute("""
        SELECT bid, cid, storage_class, cc.updated_ts,
          cc.simple_objects_count, cc.simple_objects_size, cc.multipart_objects_count,
          cc.multipart_objects_size, cc.objects_parts_count, cc.objects_parts_size,
          cc.deleted_objects_count, cc.deleted_objects_size, cc.active_multipart_count
        FROM s3.chunks_counters cc JOIN s3.chunks c USING (bid, cid)
        WHERE cc.simple_objects_size != 0 OR cc.multipart_objects_size != 0 OR cc.objects_parts_size != 0
    """)
    db.log_last_query(cur)
    return {(row.bid, row.cid, row.storage_class): row for row in cur.fetchall()}


def check_equal_counters_in_meta_and_db(metadb, s3db_map, errors_file=None):
    meta_bids = metadb.get_all_buckets_bid()
    for shard_id, db in s3db_map.items():
        cur = metadb.create_cursor(cursor_factory=NamedTupleCursor)
        cur.execute("""
            SELECT last_counters_updated_ts FROM s3.parts WHERE shard_id=%(shard_id)s
        """, {'shard_id': shard_id})
        metadb.log_last_query(cur)
        shard_last_updated_at = cur.fetchone().last_counters_updated_ts
        meta_dict = _meta_get_shard_counters(metadb, shard_id)
        meta_set = set(meta_dict.keys())
        db_dict = _db_get_counters(db)
        db_set = set(db_dict.keys())

        for (bid, cid, storage_class) in meta_set - db_set:
            actual_meta_chunk = metadb.get_chunk(bid, cid)
            if actual_meta_chunk is None or actual_meta_chunk.shard_id != shard_id:
                continue    # already moved or removed
            db_chunk_counters = [cc for cc in db.get_chunk_counters(bid, cid) if cc['storage_class'] == storage_class]
            if not db_chunk_counters:
                continue    # without counters yet
            if db_chunk_counters[0]['updated_ts'] > shard_last_updated_at:
                continue    # updated recently

            logging.warning('Presents in meta, not in %s shard: bid %s, cid %s, storage_class %s',
                            shard_id, bid, cid, storage_class)
            if errors_file:
                write_error_to_file(bid, cid, errors_file, 'excess_meta_counter')

        for (bid, cid, storage_class) in db_set - meta_set:
            if bid not in meta_bids:
                continue
            if db_dict[(bid, cid, storage_class)].updated_ts > shard_last_updated_at:
                continue
            if metadb.get_chunk(bid, cid) is None:
                continue    # already removed

            logging.warning('Presents in %s shard, not in meta: bid %s, cid %s, storage_class %s',
                            shard_id, bid, cid, storage_class)
            if errors_file:
                write_error_to_file(bid, cid, errors_file, 'missed_meta_counter')

        for (bid, cid, storage_class) in db_set.intersection(meta_set):
            meta_row = meta_dict[(bid, cid, storage_class)]
            db_row = db_dict[(bid, cid, storage_class)]
            if db_row.updated_ts > shard_last_updated_at:
                continue
            for counter in NEW_COUNTERS_KEYS:
                if getattr(meta_row, counter) != getattr(db_row, counter):
                    logging.warning('Different counters in %s shard and on meta: bid %s, cid %s, storage_class %s',
                                    shard_id, bid, cid, storage_class)
                    if errors_file:
                        write_error_to_file(bid, cid, errors_file, 'diff_counters')


def check_equal_counters_and_billing(metadb, s3db_map, period, errors_file=None):
    meta_bids = metadb.get_all_buckets_bid()
    for bid in meta_bids:
        for row in metadb.get_last_bucket_sizes(bid, period):
            db = s3db_map.get(row.shard_id)
            if not db:
                if row.size != 0:
                    logging.warning('Billing for non-existent shard: bid %s, shard_id %s, storage_class %s',
                                    bid, row.shard_id, row.storage_class)
                continue
            db_size = db.get_bid_size_by_ts(bid, row.storage_class, row.target_ts)
            if row.size != db_size:
                kind = 'perebill' if row.size > db_size else 'nedobill'
                logging.warning('Different counters and billing size: bid %s, shard_id %s, storage_class %s, %s %s',
                                bid, row.shard_id, row.storage_class, kind, row.size - db_size)
                if errors_file:
                    write_error_to_file(bid, row.storage_class, errors_file, kind)


def main():
    options, _ = parse_cmd_args()
    config = read_config(options=options)
    init_logging(config)
    logging.info("Starting %s", os.path.basename(__file__))
    pgmeta = PgmetaDB(config['pgmeta_connstring'], autocommit=True, application_name=CHECK_META_CHUNKS_COUNTERS_APPLICATION_NAME)
    metadb = S3MetaDB(config['db_connstring'], user=config.get('user'), autocommit=True, application_name=CHECK_META_CHUNKS_COUNTERS_APPLICATION_NAME)

    errors_filepath = config['errors_filepath'] if 'errors_filepath' in config else None

    if config['run_on_replica']:
        if metadb.is_master():
            logging.info("I'm master, exit.")
            return
        if not pgmeta.replica_has_less_priority(socket.gethostname(), 'meta'):
            logging.info("I'm not less priority replica, exit.")
            return
        if errors_filepath and os.path.exists(errors_filepath) and os.stat(errors_filepath).st_size != 0:
            logging.info('There is something in %s, exit' % errors_filepath)
            return

    s3db_map = {
        shard_id: pgmeta.get_replica('db', shard_id, user=config.get('user'), autocommit=True,
                                     application_name=CHECK_META_CHUNKS_COUNTERS_APPLICATION_NAME)
        for shard_id in range(0, pgmeta.get_shards_count('db'))
    }
    errors_file = open(errors_filepath, "a", buffering=0) if errors_filepath else None
    period = config['period'] if config['period'] != 'infinity' else None

    logging.info("Checking chunks bounds...")
    try:
        check_chunks_bounds(metadb, s3db_map, errors_file)
    except Exception as e:
        logging.warning('Exception occurred %s', e)

    logging.info("Checking billing buckets presence...")
    try:
        check_billing_buckets_presence(metadb, s3db_map, errors_file)
    except Exception as e:
        logging.warning('Exception occurred %s', e)

    if not config['skip_equal_buckets_usage']:
        logging.info("Checking buckets usage equality...")
        try:
            check_equal_buckets_usage_in_meta_and_db(metadb, s3db_map, period, errors_file)
        except Exception as e:
            logging.warning('Exception occurred %s', e)

    logging.info("Checking billing timeline...")
    try:
        check_billing_timeline(metadb, period, errors_file)
    except Exception as e:
        logging.warning('Exception occurred %s', e)

    logging.info("Checking deleted buckets...")
    try:
        check_deleted_buckets(metadb, s3db_map, period, errors_file)
    except Exception as e:
        logging.warning('Exception occurred %s', e)

    logging.info("Checking chunks counters equality...")
    try:
        check_equal_counters_in_meta_and_db(metadb, s3db_map, errors_file)
    except Exception as e:
        logging.warning('Exception occurred %s', e)

    logging.info("Checking chunks counters and billing equality...")
    try:
        check_equal_counters_and_billing(metadb, s3db_map, period, errors_file)
    except Exception as e:
        logging.warning('Exception occurred %s', e)

    if errors_file:
        errors_file.close()
    logging.info("Stopping %s", os.path.basename(__file__))


if __name__ == '__main__':
    main()
