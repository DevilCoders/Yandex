#!/usr/bin/env python
# encoding: utf-8

import logging
from collections import namedtuple
from optparse import OptionParser
import os
import sys
from random import sample
import psycopg2
from psycopg2.extras import NamedTupleCursor

sys.path.append(os.path.join(os.path.dirname(__file__), '../../'))
from util.s3meta import S3MetaDB
from util.helpers import init_logging, read_config
from util.const import SMART_MOVER_APPLICATION_NAME


class BidDisbalance(namedtuple('BidDisbalance', ['shard_id', 'how_much'])):
    pass


def parse_cmd_args():
    usage = """
    %prog [-t threshold] [-m min_objects] [-M max_objects] [-d db_connstring] [-p pgmeta_connstring] [-u user] [-l log_level]
    """
    parser = OptionParser(usage=usage)
    parser.add_option("-l", "--log-level", dest="log_level", default="INFO",
                      help="Verbosity level (default: %default)")
    parser.add_option("-S", "--sentry-dsn", dest="sentry_dsn",
                      help="Sentry DSN, skip if don't use")
    parser.add_option("-d", "--db-connstring", dest="db_connstring", default="dbname=s3meta",
                      help="Connection string to s3db database (default: %default)")
    parser.add_option("-u", "--user", dest="user",
                      help="User which will be used for authentication in databases")
    parser.add_option("-t", "--top-limit", dest="top_limit", default=50, type="int",
                      help="TOP-N buckets to move (default: %default)")
    parser.add_option("-b", "--bid", dest="bid",
                      help="Specify bucket need to be moved")
    parser.add_option("-p", "--priority", dest="priority", default=1, type="int",
                      help="Priority of chunks move task (default: %default)")
    parser.add_option("--min-disbalance", dest="min_disbalance", default=1.0, type="float",
                      help="Don't run if disbalance less, in percent (default: %default)")
    parser.add_option("--one-shard-threshold", dest="one_shard_threshold", default=500000, type="int",
                      help="Split into several shards if bucket has objects count more than (default: %default)")
    parser.add_option("--koeff-chunks-min", dest="koeff_chunks_min", default=100, type="int")
    parser.add_option("--koeff-shards-min", dest="koeff_shards_min", default=4, type="int")
    parser.add_option("--koeff-shards-max", dest="koeff_shards_max", default=30, type="int")
    return parser.parse_args()


def is_already_queued(metadb, bid=None):
    cur = metadb.create_cursor()
    cur.execute("""
        SELECT 1 AS c
          FROM s3.chunks_move_queue
          WHERE priority > 0
            AND (%(bid)s IS NULL OR bid = %(bid)s)
          LIMIT 1
    """, {'bid': bid})
    return bool(cur.fetchone())


def get_objects_disbalance_percent(metadb, bid=None):
    cur = metadb.create_cursor()
    cur.execute("""
        WITH s AS (
          SELECT
                shard_id,
                sum(simple_objects_count) + sum(multipart_objects_count) AS objects
              FROM s3.chunks JOIN s3.chunks_counters USING (bid, cid)
              WHERE %(bid)s IS NULL OR bid = %(bid)s
              GROUP BY shard_id
        ) SELECT (max(objects) - min(objects)) / sum(objects) FROM s;
    """, {'bid': bid})
    row = cur.fetchone()
    return row[0] * 100 if row else 0


def get_shards_to_move(metadb):
    cur = metadb.create_cursor(cursor_factory=NamedTupleCursor)
    cur.execute("""
        SELECT shard_id
          FROM s3.shard_disbalance_stat JOIN s3.parts USING (shard_id)
          WHERE new_chunks_allowed
            AND k > 0
    """)
    return {row.shard_id for row in cur.fetchall()}


def get_tomatoes(metadb):
    cur = metadb.create_cursor(cursor_factory=NamedTupleCursor)
    cur.execute("""
        SELECT bid, priority
          FROM s3.tomatoes
    """)
    return {row.bid: row.priority for row in cur.fetchall()}


def get_buckets_to_move(metadb, config):
    bid = config.get('bid')
    top_limit = config.get('top_limit') if not bid else None

    cur = metadb.create_cursor(cursor_factory=NamedTupleCursor)
    cur.execute("""
        WITH top AS ((
            SELECT bid, count(*) AS c
              FROM s3.chunks
              GROUP BY bid
              ORDER BY c DESC
              LIMIT %(top_limit)s
            ) UNION (
            SELECT bid, 0 AS c
              FROM s3.tomatoes
            )
        ),
        x AS (
          SELECT max(c) AS max_ FROM top
        ),
        koeffs AS (
          -- shards = k * chunks + b; 100 chunks => 4 shards; max chunks =>  30 shards
          SELECT
              (%(y_min)s - %(y_max)s)::float / (%(x_min)s - x.max_) AS k,
              %(y_min)s - %(x_min)s * (%(y_min)s - %(y_max)s)::float / (%(x_min)s - x.max_) AS b
            FROM x
        ),
        shards_to_bid AS (
          SELECT
              bid,
              CEIL(koeffs.k * c + koeffs.b) AS target
            FROM top, koeffs
        ),
        bid_to_stat AS (
          SELECT
              bid,
              count(distinct shard_id) AS now_shards_count,
              target AS target_shards_count,
              count(*) AS chunk_count,
              round(count(*) / target) AS target_chunks_count
            FROM s3.chunks JOIN shards_to_bid USING (bid) JOIN top USING (bid)
            GROUP BY bid, target
        ), bid_shards AS (
          SELECT
              bid,
              shard_id,
              CASE WHEN target_shards_count > now_shards_count THEN target_shards_count - now_shards_count
                   ELSE 0
                END AS need_new_shards,
              target_chunks_count,
              target_chunks_count - count(*) AS diff
            FROM s3.chunks JOIN bid_to_stat USING (bid)
            WHERE (%(bid)s IS NULL OR bid = %(bid)s)
            GROUP BY bid, shard_id, now_shards_count, target_shards_count, target_chunks_count
            HAVING target_chunks_count - count(*) < 0
            ORDER BY diff
            LIMIT %(top_limit)s
        ) SELECT bid, need_new_shards, target_chunks_count
          FROM bid_shards
          GROUP BY bid, need_new_shards, target_chunks_count
    """, {
        'top_limit': top_limit,
        'bid': bid,
        'x_min': config.get('koeff_chunks_min'),
        'y_min': config.get('koeff_shards_min'),
        'y_max': config.get('koeff_shards_max'),
    })
    metadb.log_last_query(cur)
    return [(row.bid, row.target_chunks_count, int(row.need_new_shards)) for row in cur.fetchall()]


def get_bid_disbalance(metadb, bid, target_chunks_count):
    cur = metadb.create_cursor(cursor_factory=NamedTupleCursor)
    cur.execute("""
        SELECT shard_id,
          CASE WHEN k < 0 AND to_move < 0 THEN -ROUND(k * to_move)
               WHEN k > 0 AND to_move > 0 THEN ROUND(k * to_move)
               ELSE 0
            END AS how_much
          FROM (
            SELECT
                shard_id,
                k,
                %(target_chunks_count)s - count(*) as to_move
              FROM s3.chunks JOIN s3.shard_disbalance_stat USING (shard_id)
              WHERE bid=%(bid)s
              GROUP BY shard_id, k
            ) disbalance
          ORDER BY how_much
    """, {
        'bid': bid,
        'target_chunks_count': target_chunks_count,
    })
    metadb.log_last_query(cur)
    return list(cur.fetchall())


def get_buckets_in_one_shard(metadb, config):
    cur = metadb.create_cursor(cursor_factory=NamedTupleCursor)
    cur.execute("""
        SELECT bid, shard_id, -round(chunks_count / %(shards_count)s) as how_much
            FROM (
             SELECT bid,
                    sum(simple_objects_count + multipart_objects_count) AS objects_count,
                    count(*) AS chunks_count,
                    min(shard_id) AS shard_id
               FROM s3.chunks JOIN s3.chunks_counters
               USING (bid, cid)
               GROUP BY bid
               HAVING count(distinct shard_id) = 1
            ) buckets_sizes
          WHERE objects_count >= %(threshold)s;
        """, {
        'threshold': config.get('one_shard_threshold'),
        'shards_count': config.get('koeff_shards_min'),
    })
    metadb.log_last_query(cur)
    return list(cur.fetchall())


def make_moving_tasks(bid_disbalance, need_new_shards, shards_to_move):
    result = []

    # if this bid distributed by less shards than need
    if need_new_shards:
        already_used = {row.shard_id for row in bid_disbalance}
        dst_shards = list(shards_to_move - already_used)
        if len(dst_shards) > need_new_shards:
            dst_shards = sample(dst_shards, need_new_shards)
        if len(dst_shards) > 0:
            # distribute evenly
            how_much = int(-bid_disbalance[0].how_much / (len(bid_disbalance) + len(dst_shards)))
            bid_disbalance.extend([BidDisbalance(shard, how_much) for shard in dst_shards])

    reversed_idx = len(bid_disbalance)
    for src in bid_disbalance:
        reversed_idx -= 1
        if src.how_much >= 0:
            break
        dst = bid_disbalance[reversed_idx]
        if dst.shard_id not in shards_to_move:
            continue
        count = min(-src.how_much, dst.how_much)
        if count > 0:
            result.append((src.shard_id, dst.shard_id, count))
    return result


def get_empty_chunks_to_move_for_bucket(metadb, bid, total=None):
    cur = metadb.create_cursor(cursor_factory=NamedTupleCursor)
    cur.execute("""
        WITH counters AS (
          SELECT bid, cid
            FROM s3.chunks_counters
            WHERE bid=%(bid)s
            GROUP BY bid, cid
            HAVING sum(simple_objects_count + multipart_objects_count + objects_parts_count) = 0
        ), zero AS (
          SELECT bid, cid, shard_id, start_key, end_key
            FROM s3.chunks JOIN counters
            USING (bid, cid)
        ), zero_shards AS (
          SELECT zero.bid, zero.cid, zero.shard_id, prev.shard_id AS prev_shard, next.shard_id AS next_shard
            FROM zero
              JOIN s3.chunks prev ON (zero.start_key = prev.end_key AND zero.bid = prev.bid)
              JOIN s3.chunks next ON (zero.end_key = next.start_key AND zero.bid = next.bid)
        ) SELECT bid, cid, shard_id, prev_shard, next_shard
          FROM zero_shards
          WHERE prev_shard != shard_id AND shard_id != next_shard
          LIMIT least(%(total)s, 100)
    """, {'bid': bid, 'total': total})
    metadb.log_last_query(cur)
    return cur.fetchall()


def get_empty_chunks_to_move(metadb, config):
    bid = config.get('bid')
    if bid:
        return get_empty_chunks_to_move_for_bucket(metadb, bid)

    cur = metadb.create_cursor(cursor_factory=NamedTupleCursor)
    cur.execute("""
        WITH counters AS (
          SELECT bid, cid
            FROM s3.chunks_counters
            GROUP BY bid, cid
            HAVING sum(simple_objects_count + multipart_objects_count + objects_parts_count) = 0
        ), chunks AS (
          SELECT bid, cid, shard_id
            FROM s3.chunks JOIN counters USING (bid, cid)
            WHERE (bid, cid) NOT IN (SELECT bid, cid FROM s3.chunks_move_queue)
        ) SELECT bid, count(*) AS chunks_count
          FROM chunks JOIN s3.buckets USING (bid)
          GROUP BY bid
          HAVING count(distinct shard_id) > 1 AND count(*) > 10
    """)
    metadb.log_last_query(cur)

    tasks = []
    for row in cur.fetchall():
        tasks.extend(get_empty_chunks_to_move_for_bucket(metadb, row.bid, row.chunks_count))
    return tasks


def push_to_move_queue(metadb, bid, src_shard, dst_shard, count, priority):
    cur = metadb.create_cursor()
    cur.execute("""
        SELECT v1_code.chunk_move_queue_push(shard_id, %(dst_shard)s, bid, cid, %(priority)s)
          FROM (
            SELECT bid, cid, shard_id FROM s3.chunks JOIN s3.chunks_counters USING (bid, cid)
              WHERE (simple_objects_count + multipart_objects_count > 0)
                AND bid= %(bid)s
                AND shard_id = %(src_shard)s
                AND (bid, cid) NOT IN (SELECT bid, cid FROM s3.chunks_move_queue)
          ) chunks
          GROUP BY bid, cid, shard_id
          LIMIT %(count)s
    """, {
        'dst_shard': dst_shard,
        'src_shard': src_shard,
        'bid': bid,
        'count': count,
        'priority': priority,
    })
    metadb.log_last_query(cur)
    return cur.rowcount


def chunk_move_queue_push(metadb, bid, cid, src_shard, dst_shard, priority):
    cur = metadb.create_cursor()
    cur.execute("""
        SELECT v1_code.chunk_move_queue_push(%(src_shard)s, %(dst_shard)s, %(bid)s, %(cid)s, %(priority)s)
    """, {'src_shard': src_shard, 'dst_shard': dst_shard, 'bid': bid, 'cid': cid, 'priority': priority})
    metadb.log_last_query(cur)


def start(config):
    logging.info("Starting %s", os.path.basename(__file__))
    try:
        metadb = S3MetaDB(config['db_connstring'], user=config.get('user'), application_name=SMART_MOVER_APPLICATION_NAME, autocommit=True)
    except psycopg2.OperationalError as e:
        logging.warning("Cannot connect to local server:\n%s" % str(e))
        sys.exit(1)
    if not metadb.is_master():
        logging.info("My db is replica. Nothing to do. Exiting...")
        sys.exit(0)

    if is_already_queued(metadb, config.get('bid')):
        logging.info("There are some priority tasks in queue. Exiting...")
        sys.exit(0)

    empty_chunks_tasks = get_empty_chunks_to_move(metadb, config)
    if empty_chunks_tasks:
        for task in empty_chunks_tasks:
            chunk_move_queue_push(metadb, task.bid, task.cid, task.shard_id, task.prev_shard, config.get('priority'))
        logging.info('Pushed %d tasks with empty chunks', len(empty_chunks_tasks))

    shards_to_move = get_shards_to_move(metadb)
    for row in get_buckets_in_one_shard(metadb, config):
        bid_disbalance = [BidDisbalance(row.shard_id, row.how_much) for _ in range(config.get('koeff_shards_min') - 1)]
        for src_shard, dst_shard, count in make_moving_tasks(bid_disbalance, config.get('koeff_shards_min') - 1, shards_to_move):
            count = push_to_move_queue(metadb, row.bid, src_shard, dst_shard, count, config.get('priority'))
            logging.info('Pushed to move queue %d chunks of %s from %d to %d shard', count, row.bid, src_shard, dst_shard)

    if get_objects_disbalance_percent(metadb, config.get('bid')) < config.get('min_disbalance'):
        logging.info("Everything is already good. Exiting...")
        sys.exit(0)

    tomatoes = get_tomatoes(metadb)
    for bid, target_chunks_count, need_new_shards in get_buckets_to_move(metadb, config):
        bid_disbalance = get_bid_disbalance(metadb, bid, target_chunks_count)
        priority = config.get('priority') * tomatoes.get(bid, 1)
        for src_shard, dst_shard, count in make_moving_tasks(bid_disbalance, need_new_shards, shards_to_move):
            count = push_to_move_queue(metadb, bid, src_shard, dst_shard, count, priority)
            logging.info('Pushed to move queue %d chunks of %s from %d to %d shard', count, bid, src_shard, dst_shard)

    logging.info("Stopping %s", os.path.basename(__file__))


def main():
    options, _ = parse_cmd_args()
    config = read_config(options=options)
    init_logging(config)
    start(config)


if __name__ == '__main__':
    main()
