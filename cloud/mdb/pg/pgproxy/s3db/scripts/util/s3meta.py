#!/usr/bin/env python
# encoding: utf-8

from .database import Database
from .chunk import Chunk, NEW_COUNTERS_KEYS
from .helpers import complete_tpc
from . import const
import time
import psycopg2
from psycopg2.extras import NamedTupleCursor, DictCursor
from datetime import datetime

import logging


class S3MetaDB(Database):
    def __init__(self, *args, **kwargs):
        super(S3MetaDB, self).__init__(*args, **kwargs)
        self.type = 'meta'

    def get_chunk(self, bid, cid):
        cur = self.create_cursor(cursor_factory=NamedTupleCursor)
        cur.execute("""
            SELECT * FROM s3.chunks
                WHERE bid = %(bid)s AND cid = %(cid)s
        """, {'bid': bid, 'cid': cid})
        self.log_last_query(cur)
        chunk = cur.fetchone()
        if chunk is None:
            return None
        return Chunk(
            bid=chunk.bid, cid=chunk.cid, start_key=chunk.start_key,
            end_key=chunk.end_key, created=chunk.created,
            shard_id=chunk.shard_id
        )

    def get_bucket(self, bid):
        cur = self.create_cursor(cursor_factory=NamedTupleCursor)
        cur.execute("""SELECT * FROM s3.buckets WHERE bid = %(bid)s """, {'bid': bid})
        self.log_last_query(cur)
        return cur.fetchone()

    def get_all_buckets_bid(self):
        cur = self.create_cursor()
        cur.execute("SELECT bid FROM s3.buckets")
        self.log_last_query(cur)
        return set([x[0] for x in cur.fetchall()])

    def get_deleted_buckets_bid(self, period):
        cur = self.create_cursor()
        cur.execute("""
            SELECT bid
              FROM s3.buckets_history
              WHERE deleted BETWEEN current_timestamp - %(period)s::interval AND current_timestamp
        """, {'period': period})
        self.log_last_query(cur)
        return set([x[0] for x in cur.fetchall()])

    def get_shards(self):
        cur = self.create_cursor(cursor_factory=NamedTupleCursor)
        cur.execute("""
            SELECT
                shard_id,
                last_counters_updated_ts
            FROM s3.parts
        """)
        self.log_last_query(cur)
        return cur.fetchall()

    def set_shards_updated(self, shard_id, updated_ts, chunks_counters_queue_updated_ts):
        cur = self.create_cursor()
        cur.execute("""
                UPDATE s3.parts
                SET
                  last_counters_updated_ts = %(updated_ts)s,
                  chunks_counters_queue_updated_ts = %(chunks_counters_queue_updated_ts)s
                WHERE shard_id = %(shard_id)s
            """, {
            'shard_id': shard_id,
            'updated_ts': updated_ts,
            'chunks_counters_queue_updated_ts': chunks_counters_queue_updated_ts or updated_ts,
        })
        self.log_last_query(cur)

    def get_shard_buckets_usage_last_updated(self, shard_id, except_target_ts=None):
        # Returns mapping (bid, storage_class) -> buckets usage last updated for shard
        cur = self.create_cursor(cursor_factory=NamedTupleCursor)
        cur.execute("""
            SELECT bid, storage_class, max(ts) AS last_updated
            FROM (
              SELECT bid, storage_class, max(end_ts) AS ts
                FROM s3.buckets_usage
                WHERE shard_id=%(shard_id)s
                GROUP BY bid, storage_class
              UNION
              SELECT bid, storage_class, max(target_ts) AS ts
                FROM s3.buckets_size
                WHERE shard_id=%(shard_id)s
                  AND (%(except_target_ts)s IS NULL OR target_ts <> %(except_target_ts)s)
                GROUP BY bid, storage_class
            ) l
            GROUP BY bid, storage_class
        """, {
            'shard_id': shard_id,
            'except_target_ts': except_target_ts,
        })
        self.log_last_query(cur)
        return {(row.bid, row.storage_class): row.last_updated for row in cur.fetchall()}

    def get_buckets_usage_prev_hour(self, shard_id):
        # Returns mapping (bid, storage_class, end_ts) -> (byte_secs, size_change) for previous hour
        cur = self.create_cursor(cursor_factory=NamedTupleCursor)
        cur.execute("""
            SELECT bid, storage_class, byte_secs, size_change, end_ts
            FROM s3.buckets_usage
            WHERE shard_id=%(shard_id)s
              AND end_ts = date_trunc('hour', current_timestamp)
        """, {
            'shard_id': shard_id
        })
        self.log_last_query(cur)
        return {(row.bid, row.storage_class, row.end_ts): (row.byte_secs, row.size_change) for row in cur.fetchall()}

    def get_chunks_updated_ts_by_shard(self, shard_id, cid_mod_arg=1, cid_mod_val=0):
        cur = self.create_cursor(cursor_factory=NamedTupleCursor)
        cur.execute("""
            SELECT bid, cid, max(updated_ts) as updated_ts
            FROM s3.chunks_counters JOIN s3.chunks USING (bid, cid)
            WHERE shard_id = %(shard_id)s
              AND (%(cid_mod_arg)s = 1 OR mod(cid, %(cid_mod_arg)s) = %(cid_mod_val)s)
            GROUP BY bid, cid
        """, {'shard_id': shard_id, 'cid_mod_arg': cid_mod_arg, 'cid_mod_val': cid_mod_val})
        self.log_last_query(cur)
        return {(row.bid, row.cid): row.updated_ts for row in cur.fetchall()}

    def update_chunk_counters(self, chunk_counters, chunk=None):
        context = {'storage_class': 0}    # will be replaced from chunk_counters if need
        context.update({key: 0 for key in NEW_COUNTERS_KEYS})
        if chunk:
            context.update({
                'bid': chunk.bid,
                'cid': chunk.cid,
                'updated_ts': chunk.updated_ts,
            })
        context.update(chunk_counters)

        cur = self.create_cursor()
        cur.execute("""
            INSERT INTO s3.chunks_counters AS c (
                bid, cid,
                simple_objects_count,
                simple_objects_size,
                multipart_objects_count,
                multipart_objects_size,
                objects_parts_count,
                objects_parts_size,
                deleted_objects_count,
                deleted_objects_size,
                active_multipart_count,
                storage_class,
                updated_ts,
                inserted_ts
            )
            VALUES (
                %(bid)s, %(cid)s,
                %(simple_objects_count)s, %(simple_objects_size)s,
                %(multipart_objects_count)s, %(multipart_objects_size)s,
                %(objects_parts_count)s, %(objects_parts_size)s,
                %(deleted_objects_count)s, %(deleted_objects_size)s,
                %(active_multipart_count)s, %(storage_class)s,
                %(updated_ts)s, current_timestamp
            )
            ON CONFLICT (bid, cid, storage_class) DO UPDATE
                SET simple_objects_count = EXCLUDED.simple_objects_count,
                    simple_objects_size = EXCLUDED.simple_objects_size,
                    multipart_objects_count = EXCLUDED.multipart_objects_count,
                    multipart_objects_size = EXCLUDED.multipart_objects_size,
                    objects_parts_count = EXCLUDED.objects_parts_count,
                    objects_parts_size = EXCLUDED.objects_parts_size,
                    deleted_objects_count = EXCLUDED.deleted_objects_count,
                    deleted_objects_size = EXCLUDED.deleted_objects_size,
                    active_multipart_count = EXCLUDED.active_multipart_count,
                    updated_ts = EXCLUDED.updated_ts,
                    inserted_ts = current_timestamp
                WHERE c.simple_objects_count != EXCLUDED.simple_objects_count
                    OR c.simple_objects_size != EXCLUDED.simple_objects_size
                    OR c.multipart_objects_count != EXCLUDED.multipart_objects_count
                    OR c.multipart_objects_size != EXCLUDED.multipart_objects_size
                    OR c.objects_parts_count != EXCLUDED.objects_parts_count
                    OR c.objects_parts_size != EXCLUDED.objects_parts_size
                    OR c.deleted_objects_count != EXCLUDED.deleted_objects_count
                    OR c.deleted_objects_size != EXCLUDED.deleted_objects_size
                    OR c.active_multipart_count != EXCLUDED.active_multipart_count
                    OR c.updated_ts != EXCLUDED.updated_ts
        """, context)
        self.log_last_query(cur)

    def correct_billing_data(self, shard_id, bid, storage_class, target_ts, byte_secs_diff, size_change_diff):
        cur = self.create_cursor()
        cur.execute("""
            UPDATE s3.buckets_usage
                SET size_change = size_change + %(size_change_diff)s,
                    byte_secs = byte_secs + %(byte_secs_diff)s
                WHERE shard_id=%(shard_id)s
                  AND bid = %(bid)s
                  AND storage_class = %(storage_class)s
                  AND end_ts = %(end_ts)s
        """, {
            'bid': bid,
            'shard_id': shard_id,
            'storage_class': storage_class,
            'byte_secs_diff': byte_secs_diff,
            'size_change_diff': size_change_diff,
            'end_ts': target_ts,
        })
        self.log_last_query(cur)

        cur = self.create_cursor()
        cur.execute("""
            UPDATE s3.buckets_size
                SET size = size + %(size_diff)s
                WHERE shard_id=%(shard_id)s
                  AND bid = %(bid)s
                  AND storage_class = %(storage_class)s
                  AND target_ts = %(target_ts)s
        """, {
            'bid': bid,
            'shard_id': shard_id,
            'storage_class': storage_class,
            'size_diff': size_change_diff,
            'target_ts': target_ts,
        })
        self.log_last_query(cur)

    def insert_shard_bucket_usage(self, bid, shard_id, storage_class, byte_secs, size_change, start_ts, end_ts):
        cur = self.create_cursor()
        cur.execute("""
            INSERT INTO s3.buckets_usage (
                bid, shard_id,
                storage_class, byte_secs,
                size_change, start_ts,
                end_ts
            )
            VALUES (
                %(bid)s, %(shard_id)s,
                %(storage_class)s, %(byte_secs)s,
                %(size_change)s, %(start_ts)s,
                %(end_ts)s
            )
        """, {
            'bid': bid,
            'shard_id': shard_id,
            'storage_class': storage_class,
            'byte_secs': byte_secs,
            'size_change': size_change,
            'start_ts': start_ts,
            'end_ts': end_ts,
        })
        self.log_last_query(cur)

    def insert_shard_bucket_size(self, bid, storage_class, shard_id, size, target_ts):
        cur = self.create_cursor()
        cur.execute("""
            INSERT INTO s3.buckets_size (
                bid, shard_id,
                storage_class, size, target_ts
            )
            VALUES (
                %(bid)s, %(shard_id)s,
                %(storage_class)s, %(size)s,
                %(target_ts)s
            )
        """, {
            'bid': bid,
            'shard_id': shard_id,
            'storage_class': storage_class,
            'size': size,
            'target_ts': target_ts
        })
        self.log_last_query(cur)

    def get_unused_buckets(self):
        cur = self.create_cursor(cursor_factory=DictCursor)
        cur.execute("""
            SELECT bid, shard_id
                FROM (SELECT bid
                     FROM s3.buckets b LEFT JOIN s3.chunks_counters c USING (bid)
                     WHERE c.bid is NULL
                     GROUP BY bid
                ) buckets
                JOIN s3.chunks USING(bid)
        """)
        self.log_last_query(cur)
        return cur.fetchall()

    def get_buckets_size(self, target_ts=None):
        cur = self.create_cursor(cursor_factory=DictCursor)
        # query from v1_code.update_buckets_size, see comments there
        cur.execute("""
          SELECT bid, shard_id, storage_class, size
            FROM s3.get_buckets_size_at_time(%(target_ts)s)
        """, {'target_ts': target_ts})
        self.log_last_query(cur)
        return cur.fetchall()

    def update_buckets_size(self, target_ts=None):
        cur = self.create_cursor()
        cur.execute("""
            INSERT INTO s3.buckets_size (bid, shard_id, storage_class, size, target_ts)
              SELECT bid, shard_id, storage_class, size, target_ts FROM s3.get_buckets_size_at_time(%(target_ts)s, i_raise_bad_target_ts => true)
        """, {'target_ts': target_ts})
        self.log_last_query(cur)

        if target_ts is None:
            cur.execute("SELECT date_trunc('hour', min(last_counters_updated_ts)) FROM s3.parts")
            target_ts = cur.fetchone()[0]
        cur.execute("""
            INSERT INTO s3.buckets_size (bid, shard_id, storage_class, size, target_ts)
              SELECT bid, shard_id, 0, 0, %(target_ts)s
                  FROM (
                    SELECT bid
                        FROM s3.buckets b
                          LEFT JOIN s3.buckets_size s USING (bid)
                          LEFT JOIN s3.chunks_counters c USING (bid)
                          LEFT JOIN s3.buckets_usage u USING (bid)
                        WHERE s.bid is NULL AND u.bid is NULL
                        GROUP BY bid
                        HAVING coalesce(sum(simple_objects_size), 0) = 0
                          AND coalesce(sum(multipart_objects_size), 0) = 0
                          AND coalesce(sum(objects_parts_size), 0) = 0
                  ) buckets
                  JOIN s3.chunks USING(bid)
        """, {'target_ts': target_ts})
        self.log_last_query(cur)

    def create_chunk(self, bid, start_key=None, end_key=None, shard_id=None):
        cur = self.create_cursor(cursor_factory=NamedTupleCursor)
        cur.execute("""
            SELECT * FROM v1_code.create_chunk(
                %(bid)s, %(start_key)s,
                %(end_key)s, %(shard_id)s
            )
        """, {
            'bid': bid,
            'start_key': start_key,
            'end_key': end_key,
            'shard_id': shard_id,
        })
        self.log_last_query(cur)
        chunk = cur.fetchone()
        if chunk is None:
            return None
        return Chunk(
            bid=chunk.bid, cid=chunk.cid, start_key=chunk.start_key,
            end_key=chunk.end_key, created=chunk.created,
            shard_id=chunk.shard_id
        )

    def update_chunk_boundaries(self, chunk):
        cur = self.create_cursor()
        cur.execute("""
            UPDATE s3.chunks
                SET start_key = %(start_key)s,
                    end_key = %(end_key)s
            WHERE bid = %(bid)s AND cid = %(cid)s
        """, {
            'bid': chunk.bid, 'cid': chunk.cid,
            'start_key': chunk.start_key,
            'end_key': chunk.end_key,
        })
        self.log_last_query(cur)

    def chunk_queue_pop(self):
        cur = self.create_cursor(cursor_factory=NamedTupleCursor)
        cur.execute("""
            WITH chunk AS (
                SELECT * FROM s3.chunks_create_queue
                ORDER BY created
                LIMIT 1
                FOR UPDATE SKIP LOCKED
            )
            DELETE FROM s3.chunks_create_queue q USING chunk
                WHERE q.bid = chunk.bid AND q.cid = chunk.cid
            RETURNING q.*
        """)
        self.log_last_query(cur)
        chunk = cur.fetchone()
        if chunk is None:
            return None
        return Chunk(
            bid=chunk.bid, cid=chunk.cid, start_key=chunk.start_key,
            end_key=chunk.end_key, created=chunk.created,
            shard_id=chunk.shard_id
        )

    def chunk_delete_queue_pop(self):
        cur = self.create_cursor(cursor_factory=NamedTupleCursor)
        cur.execute("""
            WITH chunk AS (
                SELECT * FROM s3.chunks_delete_queue
                ORDER BY queued
                LIMIT 1
                FOR UPDATE SKIP LOCKED
            ),
            update_counters AS (
                UPDATE s3.chunks_counters AS cc
                SET simple_objects_count = 0,
                    simple_objects_size = 0,
                    multipart_objects_count = 0,
                    multipart_objects_size = 0,
                    objects_parts_count = 0,
                    objects_parts_size = 0
                FROM chunk c
                WHERE cc.bid = c.bid AND cc.cid = c.cid
            )
            DELETE FROM s3.chunks_delete_queue q USING chunk
                WHERE q.bid = chunk.bid AND q.cid = chunk.cid
            RETURNING q.*
        """)
        self.log_last_query(cur)
        chunk = cur.fetchone()
        if chunk is None:
            return None
        return Chunk(
            bid=chunk.bid, cid=chunk.cid, start_key=chunk.start_key,
            end_key=chunk.end_key, created=chunk.created,
            shard_id=chunk.shard_id, bucket_name=chunk.bucket_name,
        )

    def refresh_shard_stat(self):
        cur = self.create_cursor()
        cur.execute("""
            SELECT v1_impl.refresh_shard_stat()
        """)
        self.log_last_query(cur)
        cur.execute("""
            SELECT v1_impl.refresh_shard_disbalance_stat()
        """)
        self.log_last_query(cur)
        cur.execute("""
            SELECT v1_impl.refresh_bucket_disbalance_stat()
        """)
        self.log_last_query(cur)

    def refresh_bucket_stat(self):
        cur = self.create_cursor()
        cur.execute("""
            SELECT v1_impl.refresh_bucket_stat()
        """)
        self.log_last_query(cur)
        cur.execute("""
            SELECT v1_impl.refresh_bucket_storage_class_stat()
        """)
        self.log_last_query(cur)

    def chunks_move_queue_pop(self, source_shard=None, dest_shard=None):
        cur = self.create_cursor(cursor_factory=NamedTupleCursor)
        cur.execute("""
            WITH task AS (
                SELECT * FROM s3.chunks_move_queue
                WHERE (%(source_shard)s IS NULL OR source_shard = %(source_shard)s)
                    AND (%(dest_shard)s IS NULL OR dest_shard = %(dest_shard)s)
                ORDER BY priority DESC, random()
                LIMIT 1
                FOR UPDATE SKIP LOCKED
            )
            DELETE FROM s3.chunks_move_queue q USING task
                WHERE q.bid = task.bid AND q.cid = task.cid
            RETURNING q.*
        """, {
            'source_shard': source_shard,
            'dest_shard': dest_shard,
        })
        self.log_last_query(cur)
        return cur.fetchone()

    def get_shards_for_chunks_move(self, max_count, allow_same_shard=False):
        cur = self.create_cursor(cursor_factory=NamedTupleCursor)
        cur.execute("""
            SELECT source_shard, dest_shard, count(*) as count, (max(priority) + 1) * count(*) AS weight
              FROM s3.chunks_move_queue
              GROUP BY source_shard, dest_shard
              ORDER BY weight DESC
        """)
        self.log_last_query(cur)
        result = []
        used_shards = set()
        max_chunks_count = 0
        for row in cur.fetchall():
            if len(result) == max_count:
                break
            if not allow_same_shard:
                if (row.source_shard in used_shards) or (row.dest_shard in used_shards):
                    continue
                used_shards.add(row.source_shard)
                used_shards.add(row.dest_shard)
            result.append((row.source_shard, row.dest_shard))
            if row.count > max_chunks_count:
                max_chunks_count = row.count
        return result, max_chunks_count

    def get_chunk_to_move(self, min_objects_diff, min_chunk_size, max_chunk_size, created_delay):
        cur = self.create_cursor(cursor_factory=NamedTupleCursor)
        cur.execute("""
            SELECT * FROM v1_code.get_chunk_to_move(
                %(min_objects_diff)s,
                %(min_chunk_size)s,
                %(max_chunk_size)s,
                %(created_delay)s
            )
        """, {
            'min_objects_diff': min_objects_diff,
            'min_chunk_size': min_chunk_size,
            'max_chunk_size': max_chunk_size,
            'created_delay': created_delay,
        })
        self.log_last_query(cur)
        return cur.fetchone()

    def chunk_set_shard(self, bid, cid, shard_id):
        cur = self.create_cursor()
        cur.execute("""
            UPDATE s3.chunks SET shard_id = %(shard_id)s
                WHERE bid = %(bid)s AND cid = %(cid)s
        """, {
            'bid': bid, 'cid': cid, 'shard_id': shard_id,
        })
        self.log_last_query(cur)

    def get_shard_by_chunk(self, bid, cid):
        cur = self.create_cursor()
        cur.execute("""
            SELECT shard_id from s3.chunks
                WHERE bid = %(bid)s AND cid = %(cid)s
        """, {
            'bid': bid, 'cid': cid
        })
        self.log_last_query(cur)
        result = cur.fetchone()
        if result:
            return result[0]
        else:
            return None

    def get_all_shard_ids(self):
        cur = self.create_cursor()
        cur.execute("SELECT shard_id from s3.parts")
        self.log_last_query(cur)
        return [x[0] for x in cur.fetchall()]

    def move_chunk(self, bid, cid, from_shard, to_shard, pgmetadb,
                   user=None, attempts=1, sleep_interval=0, fail_step=None, copy_timeout=5*60):
        tpc_name = "{0}_{1}_{2}_f{3}_t{4}".format("move", bid, cid, from_shard, to_shard)

        db_from = pgmetadb.get_master('db', from_shard, user=user, application_name=const.CHUNK_MOVER_APPLICATION_NAME)
        db_to = pgmetadb.get_master('db', to_shard, user=user, application_name=const.CHUNK_MOVER_APPLICATION_NAME)

        self.begin_tpc(tpc_name)
        db_from.begin_tpc(tpc_name)
        db_to.begin_tpc(tpc_name)
        # take advisory lock to prevent race conditions and deadlocks
        # with update_chunks_counters. Chunk_mover blocks chunk, copy him to
        # other shard and delete.
        # But this rows may be locked by update_chunks_counters.

        for attempt in range(0, const.GET_LOCK_RETRY_COUNT):
            logging.info("Taking advisory lock %s%s", str(const.CHUNKS_COUNTERS_ADVISORY_LOCK),
                         (', attempt %d' % attempt) if attempt else '')
            try:
                db_from.xact_advisory_lock(const.CHUNKS_COUNTERS_ADVISORY_LOCK)
                break
            except psycopg2.OperationalError as e:
                if e.pgcode == psycopg2.errorcodes.LOCK_NOT_AVAILABLE:
                    logging.info('Caught lock timeout. Sleep %ds and retry...', sleep_interval)
                    time.sleep(sleep_interval)
                    db_from.reconnect()
                    db_from.begin_tpc(tpc_name)
                else:
                    raise

        try:
            logging.info("Warming up chunk ('%s', %s)", bid, cid)
            tmp_chunk = db_from.get_chunk(bid, cid)
            db_from.get_object_counters(tmp_chunk, warming=True)

            logging.info("Change  shard_id for chunk ('%s', %s) from %s to %s on meta", bid, cid, from_shard, to_shard)
            self.chunk_set_shard(bid, cid, to_shard)

            logging.info("Blocking chunk ('%s', %s) on %s shard", bid, cid, from_shard)
            db_chunk = db_from.get_chunk(bid, cid, block_mode="FOR UPDATE")

            logging.info("Fetching actual counters for chunk ('%s', %s) on %s shard", bid, cid, from_shard)
            chunk_counters = db_from.get_new_chunk_counters(bid, cid)

            logging.info("Creating chunk ('%s', %s) in %s db", bid, cid, to_shard)
            db_to.create_chunk_copy(db_chunk)
            move_datetime = datetime.now()
            for cc in chunk_counters:
                db_to.insert_chunk_counters(bid, cid, cc, move_datetime)

            logging.info("Copying chunk's ('%s', %s) objects from %s to %s shard with timeout=%d s.",
                         bid, cid, from_shard, to_shard, copy_timeout)
            db_from.copy_chunk(db_to, db_chunk, copy_timeout)

            logging.info("Deleting objects from chunk ('%s', %s) objects on %s shard", bid, cid, from_shard)
            db_from.chunk_delete_objects(db_chunk)

            logging.info("Deleting chunk ('%s', %s) from %s shard", bid, cid, from_shard)
            for cc in chunk_counters:
                db_from.insert_chunk_counters(bid, cid, cc, move_datetime, negative=True)
            db_from.delete_chunk(db_chunk)
        except Exception as e:
            logging.fatal("Caught exception \"%s\"", e)
            return False

        logging.info("Completing tpc %s", tpc_name)
        complete_tpc([db_to, self, db_from], pgmetadb, attempts, sleep_interval, fail_step)
        return True

    def delete_chunk(self, chunk):
        cur = self.create_cursor()
        cur.execute("""
            DELETE FROM s3.chunks
                WHERE bid = %(bid)s AND cid = %(cid)s
        """, {
            'bid': chunk.bid,
            'cid': chunk.cid,
        })
        self.log_last_query(cur)

    def delete_chunk_counters(self, chunk):
        cur = self.create_cursor()
        cur.execute("""
            DELETE FROM s3.chunks_counters
                WHERE bid = %(bid)s AND cid = %(cid)s
        """, {
            'bid': chunk.bid,
            'cid': chunk.cid,
        })
        self.log_last_query(cur)

    def get_last_bucket_sizes(self, bid, period):
        cur = self.create_cursor(cursor_factory=NamedTupleCursor)
        cur.execute("""
          SELECT shard_id, storage_class, size, target_ts
            FROM s3.buckets_size
            WHERE bid=%(bid)s
            AND target_ts = (
              SELECT max(target_ts)
                FROM s3.buckets_size
                WHERE bid=%(bid)s AND target_ts >= current_timestamp - %(period)s::interval
            )
        """, {'bid': bid, 'period': period})
        self.log_last_query(cur)
        return cur.fetchall()
