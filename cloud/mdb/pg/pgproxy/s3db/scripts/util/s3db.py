import os
import logging
import psycopg2
import threading

from .chunk import Chunk, NEW_COUNTERS_KEYS
from .database import Database
from .helpers import complete_tpc
from .exceptions import ChunksException, ScriptException, TimeoutException
from psycopg2.extras import NamedTupleCursor, DictCursor
import psycopg2.extensions

from . import const


class S3DB(Database):
    def __init__(self, *args, **kwargs):
        super(S3DB, self).__init__(*args, **kwargs)
        self.type = 'db'
        self.exception = None

    def get_chunk(self, bid, cid, block_mode=""):
        cur = self.create_cursor(cursor_factory=NamedTupleCursor)
        cur.execute("""
            SELECT * FROM s3.chunks
                WHERE bid = %(bid)s AND cid = %(cid)s
            %(block_mode)s
        """, {
            'bid': bid,
            'cid': cid,
            'block_mode': psycopg2.extensions.AsIs(block_mode),
        })
        self.log_last_query(cur)
        chunk = cur.fetchone()
        if chunk is None:
            return None
        return Chunk(
            bid=chunk.bid, cid=chunk.cid,
            start_key=chunk.start_key, end_key=chunk.end_key,
            updated_ts=chunk.updated_ts,
        )

    def get_new_chunk_counters(self, bid, cid):
        # returns counters from s3.chunks_counters + queue
        cur = self.create_cursor(cursor_factory=DictCursor)
        cur.execute("""
            WITH chunks_data AS (
                SELECT bid, cid, storage_class,
                       simple_objects_count, simple_objects_size,
                       multipart_objects_count, multipart_objects_size,
                       objects_parts_count, objects_parts_size,
                       active_multipart_count
                  FROM s3.chunks_counters
                  WHERE bid = %(bid)s
                    AND cid = %(cid)s
            ),
            queue_data AS (
                SELECT bid, cid, storage_class,
                       coalesce(sum(simple_objects_count_change), 0) AS simple_objects_count,
                       coalesce(sum(simple_objects_size_change), 0) AS simple_objects_size,
                       coalesce(sum(multipart_objects_count_change), 0) AS multipart_objects_count,
                       coalesce(sum(multipart_objects_size_change), 0) AS multipart_objects_size,
                       coalesce(sum(objects_parts_count_change), 0) AS objects_parts_count,
                       coalesce(sum(objects_parts_size_change), 0) AS objects_parts_size,
                       coalesce(sum(active_multipart_count_change), 0) AS active_multipart_count
                  FROM s3.chunks_counters_queue
                  WHERE bid = %(bid)s
                    AND cid = %(cid)s
                  GROUP BY bid, cid, storage_class
            ),
            all_data AS (
                SELECT * FROM chunks_data
                UNION ALL
                SELECT * FROM queue_data
            )
            SELECT bid, cid, storage_class,
                   sum(simple_objects_count)::bigint AS simple_objects_count,
                   sum(simple_objects_size)::bigint AS simple_objects_size,
                   sum(multipart_objects_count)::bigint AS multipart_objects_count,
                   sum(multipart_objects_size)::bigint AS multipart_objects_size,
                   sum(objects_parts_count)::bigint AS objects_parts_count,
                   sum(objects_parts_size)::bigint AS objects_parts_size,
                   sum(active_multipart_count)::bigint AS active_multipart_count
                FROM all_data
                GROUP BY bid, cid, storage_class
        """, {
            'bid': bid,
            'cid': cid,
        })
        self.log_last_query(cur)
        return cur.fetchall()

    def insert_chunk_counters(self, bid, cid, counters, created_ts=None, negative=False):
        sign = -1 if negative else 1
        cur = self.create_cursor()
        context = {
            'bid': bid,
            'cid': cid,
            'created_ts': created_ts,
            'storage_class': counters.get('storage_class', 0),
        }
        context.update({key: sign * counters.get(key, 0) for key in NEW_COUNTERS_KEYS})
        cur.execute("""
            INSERT INTO s3.chunks_counters_queue (bid, cid,
                simple_objects_count_change, simple_objects_size_change,
                multipart_objects_count_change, multipart_objects_size_change,
                objects_parts_count_change, objects_parts_size_change,
                active_multipart_count_change, storage_class,
                created_ts)
                VALUES (%(bid)s, %(cid)s,
                    %(simple_objects_count)s, %(simple_objects_size)s,
                    %(multipart_objects_count)s, %(multipart_objects_size)s,
                    %(objects_parts_count)s, %(objects_parts_size)s,
                    %(active_multipart_count)s, %(storage_class)s,
                    coalesce(%(created_ts)s, current_timestamp)
                )
        """, context)
        self.log_last_query(cur)

    def gen_chunks(self, bid=None, limit=None, offset=None, updated_from=None, cid_mod_arg=1, cid_mod_val=0):
        cur = self.create_cursor(cursor_factory=NamedTupleCursor)
        cur.execute("""
            SELECT bid, cid, updated_ts FROM s3.chunks
            WHERE (%(bid)s IS NULL OR bid=%(bid)s)
              AND (%(updated_from)s IS NULL OR updated_ts > %(updated_from)s)
              AND (%(cid_mod_arg)s = 1 OR mod(cid, %(cid_mod_arg)s) = %(cid_mod_val)s)
            ORDER BY bid, start_key
            LIMIT %(limit)s OFFSET %(offset)s
        """, {'limit': limit, 'offset': offset, 'bid': bid, 'updated_from': updated_from,
              'cid_mod_arg': cid_mod_arg, 'cid_mod_val': cid_mod_val})

        self.log_last_query(cur)
        return [Chunk(bid=chunk.bid, cid=chunk.cid, updated_ts=chunk.updated_ts) for chunk in cur.fetchall()]

    def gen_chunks_counters(self, bid=None, limit=None, offset=None, updated_from=None, cid_mod_arg=1, cid_mod_val=0):
        cur = self.create_cursor(cursor_factory=DictCursor)
        cur.execute("""
            SELECT * FROM s3.chunks_counters
            WHERE (%(bid)s IS NULL OR bid=%(bid)s)
              AND (%(updated_from)s IS NULL OR updated_ts > %(updated_from)s)
              AND (%(cid_mod_arg)s = 1 OR mod(cid, %(cid_mod_arg)s) = %(cid_mod_val)s)
            ORDER BY bid, cid
            LIMIT %(limit)s OFFSET %(offset)s
        """, {'limit': limit, 'offset': offset, 'bid': bid, 'updated_from': updated_from,
              'cid_mod_arg': cid_mod_arg, 'cid_mod_val': cid_mod_val})

        self.log_last_query(cur)
        return cur.fetchall()

    def get_chunk_counters(self, bid, cid):
        cur = self.create_cursor(cursor_factory=DictCursor)
        cur.execute("""
            SELECT * FROM s3.chunks_counters
            WHERE bid=%(bid)s AND cid=%(cid)s
        """, {'bid': bid, 'cid': cid})
        self.log_last_query(cur)
        return cur.fetchall()

    def get_chunk_counters_correction(self, chunk, storage_class=None, with_objects=False, with_other=False):
        if with_objects:
            query = """
                SELECT
                   coalesce(sum(simple_objects_count_change), 0) AS simple_objects_count,
                   coalesce(sum(simple_objects_size_change), 0) AS simple_objects_size,
                   coalesce(sum(multipart_objects_count_change), 0) AS multipart_objects_count,
                   coalesce(sum(multipart_objects_size_change), 0) AS multipart_objects_size,
                   coalesce(sum(objects_parts_count_change), 0) AS objects_parts_count,
                   coalesce(sum(objects_parts_size_change), 0) AS objects_parts_size,
                   coalesce(sum(deleted_objects_count_change), 0) AS deleted_objects_count,
                   coalesce(sum(deleted_objects_size_change), 0) AS deleted_objects_size,
                   coalesce(sum(active_multipart_count_change), 0) AS active_multipart_count
                FROM s3.chunks_counters_queue
                WHERE bid=%(bid)s AND cid=%(cid)s
                  AND (%(storage_class)s IS NULL OR coalesce(storage_class, 0) = %(storage_class)s)
                GROUP BY bid, cid
            """
        elif with_other:
            query = """
                SELECT
                   coalesce(sum(deleted_objects_count_change), 0) AS deleted_objects_count,
                   coalesce(sum(deleted_objects_size_change), 0) AS deleted_objects_size,
                   coalesce(sum(active_multipart_count_change), 0) AS active_multipart_count
                FROM s3.chunks_counters_queue
                WHERE bid=%(bid)s AND cid=%(cid)s
                  AND (%(storage_class)s IS NULL OR coalesce(storage_class, 0) = %(storage_class)s)
                GROUP BY bid, cid
            """
        else:
            return None
        cur = self.create_cursor(cursor_factory=DictCursor)
        cur.execute(query, {'bid': chunk.bid, 'cid': chunk.cid, 'storage_class': storage_class})
        return cur.fetchone()

    def get_chunks_counters_sum(self, chunk, storage_class=None):
        """
        If storage_class is None returns sum for all storage_class
        Else returns counters for storage_class
        """
        cur = self.create_cursor(cursor_factory=DictCursor)
        cur.execute("""
            SELECT * FROM s3.chunks_counters
            WHERE bid=%(bid)s AND cid=%(cid)s
              AND (%(storage_class)s iS NULL OR storage_class = %(storage_class)s)
        """, {'bid': chunk.bid, 'cid': chunk.cid, 'storage_class': storage_class})
        self.log_last_query(cur)

        result = {key: 0 for key in NEW_COUNTERS_KEYS}
        for row in cur.fetchall():
            for key in NEW_COUNTERS_KEYS:
                result[key] += row[key]
        return result

    def get_chunks_updated_ts(self, cid_mod_arg=1, cid_mod_val=0):
        cur = self.create_cursor(cursor_factory=NamedTupleCursor)
        cur.execute("""
            SELECT bid, cid, max(cc.updated_ts) as updated_ts
            FROM s3.chunks LEFT JOIN s3.chunks_counters cc USING (bid, cid)
            WHERE (%(cid_mod_arg)s = 1 OR mod(cid, %(cid_mod_arg)s) = %(cid_mod_val)s)
            GROUP BY bid, cid
        """, {'cid_mod_arg': cid_mod_arg, 'cid_mod_val': cid_mod_val})
        self.log_last_query(cur)
        return {(row.bid, row.cid): row.updated_ts for row in cur.fetchall()}

    def insert_chunks_counters(self, chunk, chunk_counters, storage_class=0):
        cur = self.create_cursor()
        context = {
            'bid': chunk.bid,
            'cid': chunk.cid,
            'storage_class': storage_class
        }
        context.update({key: chunk_counters.get(key, 0) for key in NEW_COUNTERS_KEYS})

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
                storage_class
            )
            VALUES (
                %(bid)s, %(cid)s,
                %(simple_objects_count)s, %(simple_objects_size)s,
                %(multipart_objects_count)s, %(multipart_objects_size)s,
                %(objects_parts_count)s, %(objects_parts_size)s,
                %(deleted_objects_count)s, %(deleted_objects_size)s,
                %(active_multipart_count)s, %(storage_class)s
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
                    updated_ts = current_timestamp
                WHERE c.simple_objects_count != EXCLUDED.simple_objects_count
                    OR c.simple_objects_size != EXCLUDED.simple_objects_size
                    OR c.multipart_objects_count != EXCLUDED.multipart_objects_count
                    OR c.multipart_objects_size != EXCLUDED.multipart_objects_size
                    OR c.objects_parts_count != EXCLUDED.objects_parts_count
                    OR c.objects_parts_size != EXCLUDED.objects_parts_size
                    OR c.deleted_objects_count != EXCLUDED.deleted_objects_count
                    OR c.deleted_objects_size != EXCLUDED.deleted_objects_size
                    OR c.active_multipart_count != EXCLUDED.active_multipart_count
        """, context)
        self.log_last_query(cur)

    def gen_chunks_from_list(self, bid_cid_list):
        cur = self.create_cursor(cursor_factory=NamedTupleCursor)
        result = []
        for bid, cid in bid_cid_list:
            cur.execute("""
                SELECT bid, cid FROM s3.chunks
                WHERE bid=%(bid)s AND (%(cid)s IS NULL OR cid=%(cid)s)
            """, {'bid': bid, 'cid': cid})
            self.log_last_query(cur)
            for chunk in cur.fetchall():
                result.append(Chunk(bid=chunk.bid, cid=chunk.cid))
        return result

    def get_object_counters(self, chunk, storage_class=None, warming=False):
        if not warming:
            self.ensure_repeatable_read()
        cur = self.create_cursor(cursor_factory=NamedTupleCursor)
        cur.execute("""
            SELECT
                coalesce(
                    sum(data_size) FILTER (WHERE parts_count=0),
                    0
                )::bigint AS simple_objects_size,
                count(1) FILTER (WHERE parts_count=0) AS simple_objects_count,
                coalesce(
                    sum(data_size) FILTER (WHERE parts_count > 0),
                    0
                )::bigint AS multipart_objects_size,
                count(1) FILTER (WHERE parts_count > 0) AS multipart_objects_count,
                coalesce(
                    sum(parts_count) FILTER (WHERE parts_inline=0),
                    0
                ) AS completed_parts_count
            FROM (
                SELECT
                    name, data_size, coalesce(parts_count, 0) AS parts_count,
                    coalesce(parts_inline, 0) AS parts_inline
                FROM (
                    SELECT name, data_size, parts_count, array_length(parts, 1) AS parts_inline
                        FROM s3.objects
                        WHERE bid = %(bid)s
                            AND (%(start_key)s IS NULL OR name >= %(start_key)s)
                            AND (%(end_key)s IS NULL OR name < %(end_key)s)
                            AND (%(storage_class)s IS NULL OR coalesce(storage_class, 0) = %(storage_class)s)
                    UNION ALL
                    SELECT name,
                        CASE WHEN delete_marker THEN length(name) ELSE data_size END AS data_size,
                        parts_count, array_length(parts, 1) AS parts_inline
                        FROM s3.objects_noncurrent
                        WHERE bid = %(bid)s
                            AND (%(start_key)s IS NULL OR name >= %(start_key)s)
                            AND (%(end_key)s IS NULL OR name < %(end_key)s)
                            AND (%(storage_class)s IS NULL OR coalesce(storage_class, 0) = %(storage_class)s)
                    UNION ALL
                    SELECT name, length(name) AS data_size, 0 AS parts_count, 0 AS parts_inline
                        FROM s3.object_delete_markers
                        WHERE bid = %(bid)s
                            AND (%(start_key)s IS NULL OR name >= %(start_key)s)
                            AND (%(end_key)s IS NULL OR name < %(end_key)s)
                            AND (coalesce(%(storage_class)s, 0) = 0)
                ) as a
            ) as a
        """, {'bid': chunk.bid, 'start_key': chunk.start_key, 'end_key': chunk.end_key, 'storage_class': storage_class})
        row_objects = cur.fetchone()
        cur.execute("""
            SELECT
                coalesce(
                    sum(data_size) FILTER (WHERE part_id > 0),
                    0
                )::bigint AS objects_parts_size,
                count(1) FILTER (WHERE part_id > 0) AS objects_parts_count
            FROM s3.object_parts
            WHERE
                bid = %(bid)s
                AND (%(start_key)s IS NULL OR name >= %(start_key)s)
                AND (%(end_key)s IS NULL OR name < %(end_key)s)
                AND (%(storage_class)s IS NULL OR coalesce(storage_class, 0) = %(storage_class)s)
        """, {'bid': chunk.bid, 'start_key': chunk.start_key, 'end_key': chunk.end_key, 'storage_class': storage_class})
        row_parts = cur.fetchone()
        return {
            'simple_objects_count': row_objects.simple_objects_count,
            'simple_objects_size': row_objects.simple_objects_size,
            'multipart_objects_count': row_objects.multipart_objects_count,
            'multipart_objects_size': row_objects.multipart_objects_size,
            'completed_parts_count': row_objects.completed_parts_count,
            'objects_parts_count': row_parts.objects_parts_count,
            'objects_parts_size': row_parts.objects_parts_size,
        }

    def get_completed_parts_count(self, chunk, storage_class=None):
        self.ensure_repeatable_read()
        cur = self.create_cursor(cursor_factory=NamedTupleCursor)
        cur.execute("""
            SELECT count(*) completed_parts_count
            FROM s3.completed_parts
            WHERE bid = %(bid)s
                AND (%(start_key)s IS NULL OR name >= %(start_key)s)
                AND (%(end_key)s IS NULL OR name < %(end_key)s)
                AND (%(storage_class)s IS NULL OR coalesce(storage_class, 0) = %(storage_class)s)
        """, {'bid': chunk.bid, 'start_key': chunk.start_key, 'end_key': chunk.end_key, 'storage_class': storage_class})
        row = cur.fetchone()
        return row.completed_parts_count

    def repair_chunk_counters(self, chunk):
        cur = self.create_cursor()
        cur.execute("""
            SELECT util.repair_chunks_counters(%(bid)s, %(cid)s)
        """, {'bid': chunk.bid, 'cid': chunk.cid})
        self.log_last_query(cur)

    def get_bloated_chunks(self, threshold, bid=None, cid=None):
        cur = self.create_cursor(cursor_factory=NamedTupleCursor)
        cur.execute("""
            SELECT bid, cid, start_key, end_key
            FROM s3.chunks_counters JOIN s3.chunks USING (bid, cid)
            WHERE (%(bid)s IS NULL OR bid = %(bid)s)
                AND (%(cid)s IS NULL OR cid = %(cid)s)
                AND simple_objects_count + multipart_objects_count > %(threshold)s
            ORDER BY simple_objects_count + multipart_objects_count DESC
        """, {
            'threshold': threshold,
            'bid': bid,
            'cid': cid,
        })
        self.log_last_query(cur)

        result = []
        for chunk in list(cur.fetchall()):
            cur.execute("""
                SELECT 1
                    FROM s3.objects
                    WHERE bid = %(bid)s
                        AND (%(start_key)s IS NULL OR name >= %(start_key)s)
                        AND (%(end_key)s IS NULL OR name < %(end_key)s)
                    LIMIT 2
            """, {
                'bid': chunk.bid,
                'start_key': chunk.start_key,
                'end_key': chunk.end_key
            })
            if cur.rowcount == 2:
                # at least 2 objects in chunk
                result.append(Chunk(bid=chunk.bid, cid=chunk.cid))
        return result

    def create_chunk_copy(self, chunk):
        return self.create_chunk(chunk.bid, chunk.cid, chunk.start_key, chunk.end_key, chunk.updated_ts)

    def create_chunk(self, bid, cid, start_key, end_key, updated_ts):
        cur = self.create_cursor()
        cur.execute("""
            INSERT INTO s3.chunks (bid, cid, start_key, end_key, updated_ts)
                VALUES (%(bid)s, %(cid)s,  %(start_key)s, %(end_key)s, %(updated_ts)s)
        """, {
            'bid': bid,
            'cid': cid,
            'start_key': start_key,
            'end_key': end_key,
            'updated_ts': updated_ts,
        })
        self.log_last_query(cur)

    def chunk_delete_objects(self, chunk):
        cur = self.create_cursor()
        cur.execute("""
            WITH
                deleted_objects AS (
                    DELETE FROM s3.objects
                    WHERE bid = %(bid)s
                      AND (%(start_key)s IS NULL OR name >= %(start_key)s)
                      AND (%(end_key)s IS NULL OR name < %(end_key)s)
                    RETURNING *
                ),
                deleted_objects_noncurrent AS (
                    DELETE FROM s3.objects_noncurrent
                    WHERE bid = %(bid)s
                      AND (%(start_key)s IS NULL OR name >= %(start_key)s)
                      AND (%(end_key)s IS NULL OR name < %(end_key)s)
                    RETURNING *
                ),
                deleted_completed_parts AS (
                    DELETE FROM s3.completed_parts
                    WHERE bid = %(bid)s
                      AND (%(start_key)s IS NULL OR name >= %(start_key)s)
                      AND (%(end_key)s IS NULL OR name < %(end_key)s)
                    RETURNING *
                ),
                deleted_delete_markers AS (
                    DELETE FROM s3.object_delete_markers
                    WHERE bid = %(bid)s
                      AND (%(start_key)s IS NULL OR name >= %(start_key)s)
                      AND (%(end_key)s IS NULL OR name < %(end_key)s)
                    RETURNING *
                ),
                deleted_object_parts AS (
                    DELETE FROM s3.object_parts
                    WHERE bid = %(bid)s
                      AND (%(start_key)s IS NULL OR name >= %(start_key)s)
                      AND (%(end_key)s IS NULL OR name < %(end_key)s)
                    RETURNING *
                )
            SELECT COUNT(*)
            FROM deleted_objects
        """, {
            'bid': chunk.bid,
            'start_key': chunk.start_key,
            'end_key': chunk.end_key,
        })
        self.log_last_query(cur)
        numobjects = cur.fetchone()[0]
        if numobjects < 0:
            raise ScriptException("Deleted {0} objects from chunk ('{1}', {2})".format(
                numobjects, chunk.bid, chunk.cid))
        return numobjects

    def delete_chunk(self, chunk):
        # Make sure that you inserted negative counters for this chunk to queue
        cur = self.create_cursor()
        cur.execute("""
            DELETE FROM s3.chunks
                WHERE bid = %(bid)s AND cid = %(cid)s
        """, {
            'bid': chunk.bid,
            'cid': chunk.cid,
        })
        self.log_last_query(cur)
        if cur.rowcount != 1:
            raise ChunksException("Delete chunk affected {0} rows".format(cur.rowcount))

    def is_chunk_exists(self, bid, cid):
        cur = self.create_cursor()
        cur.execute(
            "SELECT count(*) FROM s3.chunks WHERE bid = %(bid)s AND cid = %(cid)s",
            {'bid': bid, 'cid': cid}
        )
        self.log_last_query(cur)
        count = cur.fetchone()[0]
        if count > 1:
            raise ChunksException('chunks count with bid {bid} and {cid} > 1'.format(bid=bid, cid=cid))
        return count == 1

    def copy_chunk(self, db_to, chunk, timeout):
        def copy_from(table):
            try:
                cursor = db_to.create_cursor()
                cursor.copy_from(os.fdopen(r_fd), table)
                db_to.log_last_query(cursor)
                cursor.close()
            except Exception as e:
                self.exception = e

        for table in ["objects", "object_parts", "object_delete_markers", "objects_noncurrent", "completed_parts", "inflights"]:
            cur = self.create_cursor()
            cur.execute("""
                SELECT
                    string_agg(
                        quote_ident(column_name),
                        E',\\n' ORDER BY ordinal_position
                    )
                FROM
                    information_schema.columns
                WHERE
                    table_schema = 's3'
                    AND table_name = %(table_name)s
            """, {
                'table_name': table,
                'cid': chunk.cid
            })
            self.log_last_query(cur)
            columns = cur.fetchone()[0]
            r_fd, w_fd = os.pipe()
            # New thread will execute 'COPY TO "table"' from pipe in db_to
            # and main thread will execute 'COPY "table"' to pipe in current db
            thread = threading.Thread(
                target=copy_from,
                kwargs={'table': '"s3"."{0}"'.format(table.replace('"', '""'))}
            )
            thread.start()
            write_f = os.fdopen(w_fd, 'w')
            cur.copy_expert(
                cur.mogrify("""
                    COPY (
                        SELECT
                            %(columns)s
                        FROM
                            "s3".%(table)s
                        WHERE
                            bid = %(bid)s
                            AND (%(start_key)s IS NULL OR name >= %(start_key)s)
                            AND (%(end_key)s IS NULL OR name < %(end_key)s)
                    ) TO STDOUT
                """, {
                    'bid': chunk.bid,
                    'start_key': chunk.start_key,
                    'end_key': chunk.end_key,
                    'columns': psycopg2.extensions.AsIs(columns.replace('cid', 'NULL')),
                    'table': psycopg2.extensions.AsIs(table),
                }),
                write_f
            )
            self.log_last_query(cur)
            write_f.close()
            thread.join(timeout=timeout)
            if thread.is_alive():
                raise TimeoutException('Copy timeout')
            thread_exception = self.exception
            self.exception = None
            if thread_exception:
                raise thread_exception

    def split_chunk(self, chunk, pgmetadb, meta_user=None, attempts=1, sleep_interval=0, fail_step=None):
        # fail_step (int) - emulate fail after specified commit. Used for testing
        tpc_prefix = "split"
        split_key = chunk.split_key
        split_limit = chunk.split_limit

        if split_key is not None:
            logging.info("Splitting chunk %s of '%s' by key '%s'",
                         chunk.cid, chunk.bid, split_key)
        else:
            logging.info("Splitting chunk %s of '%s' by limit '%s'",
                         chunk.cid, chunk.bid, split_limit)

        connstring_kwargs = {'application_name': const.CHUNK_SPLITTER_APPLICATION_NAME}
        if meta_user:
            connstring_kwargs['user'] = meta_user
        metadb, bucket = pgmetadb.get_s3meta_rw_by_bucket(chunk.bid, **connstring_kwargs)
        metadb.commit()
        if not metadb:
            logging.error("Bucket by bid %s not found", chunk.bid)
            return False
        logging.debug(metadb)
        logging.debug("Found bucket name '%s'", bucket.name)
        metadb.begin_tpc("{0}_{1}_{2}".format(tpc_prefix, chunk.bid, chunk.cid))

        # We need to acquire advisory lock on bucket to prevent
        # deadlocks while concurrent splitting different chunks
        # of one bucket due to deffered exclusion constraint.
        metadb.xact_advisory_lock(chunk.bid)

        chunk = metadb.get_chunk(chunk.bid, chunk.cid)
        new_chunk = metadb.create_chunk(chunk.bid, shard_id=chunk.shard_id)
        logging.debug("New chunk id is %s", new_chunk.cid)

        self.shard_id = chunk.shard_id

        dbs = [self, metadb]

        self.begin_tpc("{0}_{1}_{2}_{3}".format(tpc_prefix, chunk.bid, chunk.cid, new_chunk.cid))
        cur = self.create_cursor(cursor_factory=NamedTupleCursor)
        try:
            cur.execute("""
                SELECT * FROM v1_code.split_chunk(
                    %(bid)s, %(cid)s, %(new_cid)s, %(split_limit)s, %(split_key)s
                )
            """, {
                'bid': chunk.bid,
                'cid': chunk.cid,
                'new_cid': new_chunk.cid,
                'split_limit': split_limit,
                'split_key': split_key,
            })
        except psycopg2.IntegrityError as e:    # suppose check_key_range_empty
            logging.error('Skip because of empty key range. Please, check counters. Error: %s', e)
            self.reconnect()
            metadb.reconnect()
            return False
        except psycopg2.OperationalError as e:
            if e.pgcode == psycopg2.errorcodes.LOCK_NOT_AVAILABLE:
                logging.warning('Skip because of lock timeout.')
                self.reconnect()
                metadb.reconnect()
                return False
            raise
        self.log_last_query(cur)
        new_db_chunk = cur.fetchone()
        chunk.end_key = new_db_chunk.start_key
        metadb.update_chunk_boundaries(chunk)
        metadb.update_chunk_boundaries(new_db_chunk)
        try:
            complete_tpc(dbs, pgmetadb, attempts, sleep_interval, fail_step)
        except psycopg2.IntegrityError as e:
            if e.pgcode == psycopg2.errorcodes.EXCLUSION_VIOLATION:     # suppose idx_exclude_bid_key_range
                logging.error('Skip because of conflicting keys boundaries. Please, check counters. Error: %s', e)
                self.reconnect()
                metadb.reconnect()
                return False
            raise
        logging.info('Done.')
        return True

    def add_chunk(self, chunk):
        cur = self.create_cursor()
        cur.execute("""
            SELECT v1_code.add_chunk(
                %(bid)s, %(cid)s,
                %(start_key)s, %(end_key)s
            )
        """, {
            'bid': chunk.bid,
            'cid': chunk.cid,
            'start_key': chunk.start_key,
            'end_key': chunk.end_key,
        })
        self.log_last_query(cur)

    def update_chunks_counters(self, batch_size=None):
        # Take advisory lock to prevent race conditions and deadlocks
        # with chunk_mover. Chunk_mover blocks chunk, copies it to
        # other shard and deletes it.
        # But this rows may be locked by update_chunks_counters.
        self.xact_advisory_lock(const.CHUNKS_COUNTERS_ADVISORY_LOCK)
        cur = self.create_cursor(cursor_factory=NamedTupleCursor)
        cur.execute("""
            SELECT * FROM util.update_chunks_counters(%(batch_size)s)
        """, {'batch_size': batch_size})
        self.log_last_query(cur)
        return cur.fetchone()

    def move_objects_to_delete_queue(self, ttl, batch_size):
        cur = self.create_cursor()
        cur.execute("""
            SELECT count(1) FROM util.move_objects_to_delete_queue(
                    %(ttl)s, %(batch_size)s
            )""", {'ttl': ttl, 'batch_size': batch_size})
        self.log_last_query(cur)
        return cur.fetchone()[0]

    def move_objects_parts_to_delete_queue(self, ttl, batch_size):
        cur = self.create_cursor()
        cur.execute("""
            SELECT count(1) FROM util.move_object_parts_to_delete_queue(
                    %(ttl)s, %(batch_size)s
            )""", {'ttl': ttl, 'batch_size': batch_size})
        self.log_last_query(cur)
        return cur.fetchone()[0]

    def copy_to_billing_delete_queue(self, free_time, last_ts, storage_class):
        """
        Copy rows (lasted less than `free_time`)
            from s3.storage_delete_queue to s3.billing_delete_queue.
        """
        cur = self.create_cursor()
        cur.execute("""
            INSERT INTO s3.billing_delete_queue(bid, name, part_id, data_size, deleted_ts, created, storage_class)
            SELECT bid, name, part_id, data_size, deleted_ts, created, storage_class
            FROM s3.storage_delete_queue AS storage_q
            WHERE storage_class=%(storage_class)s
            AND deleted_ts >= TO_TIMESTAMP(%(last_ts)s)
            AND storage_q.deleted_ts - storage_q.created < interval %(t)s
            ON CONFLICT DO NOTHING
        """, {
            't': free_time,
            'last_ts': last_ts,
            'storage_class': storage_class
        })

        self.log_last_query(cur)

    def chunk_clean(self, chunk):
        """
        Remove chunk objects by clean method (move to storage delete queue)
        We assume that there are no many objects in chunk, because
        proxy checks that bucket is empty and there may be only objects
        concurrently uploaded at the same time.
        """
        cur = self.create_cursor()
        cur.execute("""
            SELECT count(*)
            FROM (
                SELECT bid, name
                FROM s3.objects
                WHERE bid = %(bid)s
                    AND (%(start_key)s IS NULL OR name >= %(start_key)s)
                    AND (%(end_key)s IS NULL OR name < %(end_key)s)
            ) o,
            LATERAL v1_code.drop_object(bid, 'disabled', name)
        """, {
            'bid': chunk.bid,
            'start_key': chunk.start_key,
            'end_key': chunk.end_key,
        })
        self.log_last_query(cur)
        numobjects = cur.fetchone()[0]
        cur.execute("""
            SELECT count(v1_code.abort_multipart_upload('', bid, name, object_created))
            FROM s3.object_parts
            WHERE bid = %(bid)s
                AND (%(start_key)s IS NULL OR name >= %(start_key)s)
                AND (%(end_key)s IS NULL OR name < %(end_key)s)
                AND part_id = 0
        """, {
            'bid': chunk.bid,
            'start_key': chunk.start_key,
            'end_key': chunk.end_key,
        })
        numparts = cur.fetchone()[0]
        return (numobjects, numparts)

    def get_chunks_counters_queue_updated_before(self):
        cur = self.create_cursor()
        cur.execute("""
          SELECT coalesce(created_ts, current_timestamp)
            FROM current_timestamp
            LEFT JOIN (
              SELECT created_ts FROM s3.chunks_counters_queue ORDER BY id LIMIT 1
            ) a ON true
        """)
        return cur.fetchone()[0]

    def get_ready_buckets_usage(self, bid_storage_class_to_last_updated, ignore_queue=False):
        # ignore_queue for manual update by fill_buckets_size
        end_to = self.get_chunks_counters_queue_updated_before() if not ignore_queue else None
        cur = self.create_cursor(cursor_factory=NamedTupleCursor)
        cur.execute("""
            SELECT bid, storage_class, byte_secs, size_change, start_ts, end_ts
            FROM s3.buckets_usage
            WHERE end_ts < coalesce(%(end_to)s, current_timestamp)
        """, {'end_to': end_to})

        self.log_last_query(cur)
        result = []
        for row in cur.fetchall():
            min_start_ts = bid_storage_class_to_last_updated.get((row.bid, row.storage_class))
            if min_start_ts and row.start_ts < min_start_ts:
                # skip because already stored in metadb, append only new rows
                continue
            result.append({
                'bid': row.bid,
                'storage_class': row.storage_class,
                'byte_secs': row.byte_secs,
                'size_change': row.size_change,
                'start_ts': row.start_ts,
                'end_ts': row.end_ts,
            })
        result.sort(key=lambda x: x['start_ts'])
        return result

    def get_buckets_usage_prev_hour(self):
        # Returns mapping (bid, storage_class, end_ts) -> (byte_secs, size_change) for previous hour
        cur = self.create_cursor(cursor_factory=NamedTupleCursor)
        cur.execute("""
            SELECT bid, storage_class, byte_secs, size_change, end_ts
            FROM s3.buckets_usage
            WHERE end_ts = date_trunc('hour', current_timestamp)
        """)
        self.log_last_query(cur)
        return {(row.bid, row.storage_class, row.end_ts): (row.byte_secs, row.size_change) for row in cur.fetchall()}

    def get_buckets_size(self, target_ts):
        cur = self.create_cursor()
        cur.execute("""
            SELECT bid, updated_ts FROM s3.chunks_counters WHERE updated_ts > %(target_ts)s LIMIT 1
        """, {'target_ts': target_ts})
        err_row = cur.fetchone()
        if err_row:
            err_bid, err_updated_ts = err_row
            raise ScriptException('Cannot get exact size of bucket {bid} because updated_ts {updated_ts} > target_ts '
                                  '{target_ts}'.format(bid=err_bid, updated_ts=err_updated_ts, target_ts=target_ts))

        cur = self.create_cursor(cursor_factory=DictCursor)
        cur.execute("""
            SELECT bid, storage_class, size FROM v1_code.get_buckets_size_at_time(%(target_ts)s)
        """, {'target_ts': target_ts})

        self.log_last_query(cur)
        return cur.fetchall()

    def get_object_deleted_counters(self, chunk, storage_class=None):
        # Returns deleted counters from s3.storage_delete_queue
        self.ensure_repeatable_read()
        cur = self.create_cursor(cursor_factory=NamedTupleCursor)
        cur.execute("""
            SELECT
               coalesce(count(1), 0) as deleted_objects_count,
               coalesce(sum(data_size), 0) as deleted_objects_size
            FROM s3.storage_delete_queue
            WHERE
               bid = %(bid)s
               AND (%(start_key)s IS NULL OR name >= %(start_key)s)
               AND (%(end_key)s IS NULL OR name < %(end_key)s)
               AND (%(storage_class)s IS NULL OR coalesce(storage_class, 0) = %(storage_class)s)
       """, {'bid': chunk.bid, 'start_key': chunk.start_key, 'end_key': chunk.end_key, 'storage_class': storage_class})
        self.log_last_query(cur)
        row = cur.fetchone()
        return (row.deleted_objects_count, row.deleted_objects_size) if row else (0, 0)

    def get_object_active_multipart_count(self, chunk, storage_class=None):
        # Returns active_multipart_count from s3.object_parts
        self.ensure_repeatable_read()
        cur = self.create_cursor(cursor_factory=NamedTupleCursor)
        cur.execute("""
             SELECT
                coalesce(count(1), 0) as active_multipart_count
             FROM s3.object_parts
             WHERE
                part_id = 0
                AND bid = %(bid)s
                AND (%(start_key)s IS NULL OR name >= %(start_key)s)
                AND (%(end_key)s IS NULL OR name < %(end_key)s)
                AND (%(storage_class)s IS NULL OR coalesce(storage_class, 0) = %(storage_class)s)
        """, {'bid': chunk.bid, 'start_key': chunk.start_key, 'end_key': chunk.end_key, 'storage_class': storage_class})
        self.log_last_query(cur)
        row = cur.fetchone()
        return row.active_multipart_count if row else 0

    def get_deleted_diff_count_and_size(self, bid, storage_class=None):
        # Returns deleted counters diff (real - in counters) by bid
        self.ensure_repeatable_read()
        cur = self.create_cursor(cursor_factory=NamedTupleCursor)
        cur.execute("""
            SELECT
               coalesce(count(1), 0) as deleted_objects_count,
               coalesce(sum(data_size), 0) as deleted_objects_size
            FROM s3.storage_delete_queue
            WHERE
               bid = %(bid)s
               AND (%(storage_class)s IS NULL OR coalesce(storage_class, 0) = %(storage_class)s)
       """, {'bid': bid, 'storage_class': storage_class})
        self.log_last_query(cur)
        row = cur.fetchone()
        count_real, size_real = (row.deleted_objects_count, row.deleted_objects_size) if row else (0, 0)

        cur = self.create_cursor(cursor_factory=NamedTupleCursor)
        cur.execute("""
            SELECT
               coalesce(sum(deleted_objects_count_change), 0) AS deleted_objects_count,
               coalesce(sum(deleted_objects_size_change), 0) AS deleted_objects_size
            FROM s3.chunks_counters_queue
            WHERE bid=%(bid)s
              AND (%(storage_class)s iS NULL OR storage_class = %(storage_class)s)
        """, {'bid': bid, 'storage_class': storage_class})
        self.log_last_query(cur)
        row = cur.fetchone()
        count_pending, size_pending = (row.deleted_objects_count, row.deleted_objects_size) if row else (0, 0)

        cur = self.create_cursor(cursor_factory=NamedTupleCursor)
        cur.execute("""
            SELECT
               coalesce(sum(deleted_objects_count), 0) AS deleted_objects_count,
               coalesce(sum(deleted_objects_size), 0) AS deleted_objects_size
            FROM s3.chunks_counters
            WHERE bid=%(bid)s
              AND (%(storage_class)s iS NULL OR storage_class = %(storage_class)s)
        """, {'bid': bid, 'storage_class': storage_class})
        self.log_last_query(cur)
        row = cur.fetchone()
        count, size = (row.deleted_objects_count, row.deleted_objects_size) if row else (0, 0)
        return count_real - count - count_pending, size_real - size - size_pending

    def insert_fake_deleted_counters(self, bid, deleted_objects_count, deleted_objects_size, storage_class=0):
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
                storage_class
            )
            VALUES (
                %(bid)s, -1,
                0, 0, 0, 0, 0, 0,
                %(deleted_objects_count)s, %(deleted_objects_size)s,
                0, %(storage_class)s
            )
            ON CONFLICT (bid, cid, storage_class) DO UPDATE
                SET deleted_objects_count = c.deleted_objects_count + EXCLUDED.deleted_objects_count,
                    deleted_objects_size = c.deleted_objects_size + EXCLUDED.deleted_objects_size,
                    updated_ts = current_timestamp
        """, {
            'bid': bid,
            'storage_class': storage_class,
            'deleted_objects_count': deleted_objects_count,
            'deleted_objects_size': deleted_objects_size
        })
        self.log_last_query(cur)

    def get_chunk_storage_classes(self, bid, cid):
        cur = self.create_cursor()
        cur.execute("""
            SELECT coalesce(storage_class, 0) FROM s3.chunks_counters WHERE bid = %(bid)s AND cid=%(cid)s
            UNION
            SELECT coalesce(storage_class, 0) FROM s3.chunks_counters_queue WHERE bid = %(bid)s AND cid=%(cid)s
        """, {'bid': bid, 'cid': cid})
        return [row[0] for row in cur.fetchall()]

    def get_active_multipart_counter(self, bid, cid):
        cur = self.create_cursor()
        cur.execute("""
          SELECT sum(active_multipart_count) FROM (
              SELECT active_multipart_count
                FROM s3.chunks_counters
                WHERE bid = %(bid)s AND cid=%(cid)s AND active_multipart_count != 0
              UNION ALL
              SELECT active_multipart_count_change AS active_multipart_count
                FROM s3.chunks_counters_queue
                WHERE bid = %(bid)s AND cid=%(cid)s AND active_multipart_count_change != 0
          ) u
        """, {'bid': bid, 'cid': cid})
        return cur.fetchone()[0] or 0

    def clear_zero_chunks_counters(self, bid=None):
        cur = self.create_cursor(cursor_factory=NamedTupleCursor)
        cur.execute("""
            DELETE FROM s3.chunks_counters
                WHERE
                  (%(bid)s IS NULL OR bid = %(bid)s)
                  AND simple_objects_count = 0
                  AND simple_objects_size = 0
                  AND multipart_objects_count = 0
                  AND multipart_objects_size = 0
                  AND objects_parts_count = 0
                  AND objects_parts_size = 0
                  AND deleted_objects_count = 0
                  AND deleted_objects_size = 0
                  AND (bid, cid) NOT IN (
                    SELECT bid, cid FROM s3.chunks
                  )
                  AND (bid, cid) NOT IN (
                    SELECT DISTINCT bid, cid FROM s3.chunks_counters_queue
                  )
        """, {'bid': bid})
        self.log_last_query(cur)

    def get_zero_chunks(self, bid=None, cid=None):
        cur = self.create_cursor(cursor_factory=NamedTupleCursor)
        cur.execute("""
            SELECT bid, cid
            FROM s3.chunks LEFT JOIN s3.chunks_counters USING (bid, cid)
            WHERE (%(bid)s IS NULL OR bid = %(bid)s)
                AND (%(cid)s IS NULL OR cid = %(cid)s)
                AND cid != -1
            GROUP BY (bid, cid)
            HAVING sum(coalesce(simple_objects_count, 0) +
                       coalesce(multipart_objects_count, 0) +
                       coalesce(objects_parts_count, 0)) = 0
        """, {
            'bid': bid,
            'cid': cid,
        })
        self.log_last_query(cur)
        return [(row.bid, row.cid) for row in cur.fetchall()]

    def get_chunk_merge_to(self, bid, cid, only_check=False):
        # Can't merge if s3.chunks_counters updated recently
        cur = self.create_cursor(cursor_factory=NamedTupleCursor)
        cur.execute("""
            SELECT 1
            FROM s3.chunks_counters
            WHERE bid = %(bid)s
                AND cid = %(cid)s
                AND updated_ts > current_timestamp - '12hours'::interval
        """, {'bid': bid, 'cid': cid})
        self.log_last_query(cur)
        if cur.fetchone():
            if only_check:
                return False, 'chunks_counters_updated_recently'
            return

        # Can't merge if s3.chunks created or updated recently
        cur = self.create_cursor(cursor_factory=NamedTupleCursor)
        cur.execute("""
            SELECT start_key
            FROM s3.chunks
            WHERE bid = %(bid)s
                AND cid = %(cid)s
                AND updated_ts < current_timestamp - '12hours'::interval
        """, {'bid': bid, 'cid': cid})
        self.log_last_query(cur)
        row = cur.fetchone()
        if not row:
            if only_check:
                return False, 'chunks_updated_recently'
            return

        # Can't merge if it's first chunk in bucket
        start_key = row.start_key
        if start_key is None:
            if only_check:
                return False, 'first_chunk_in_bucket'
            return

        # Can't merge if previous chunk not in this shard
        cur = self.create_cursor(cursor_factory=NamedTupleCursor)
        cur.execute("""
            SELECT *
            FROM s3.chunks
            WHERE bid = %(bid)s
                AND end_key = %(end_key)s
        """, {'bid': bid, 'end_key': start_key})
        self.log_last_query(cur)
        chunk = cur.fetchone()
        if not chunk:
            if only_check:
                return False, 'prev_chunk_in_another_shard'
            return

        # Can't merge if there is something in counters queue
        cur = self.create_cursor(cursor_factory=NamedTupleCursor)
        cur.execute("""
            SELECT 1
            FROM s3.chunks_counters_queue
            WHERE bid = %(bid)s
                AND cid = %(cid)s
                AND (simple_objects_count_change > 0
                  OR multipart_objects_count_change > 0
                  OR objects_parts_count_change > 0)
            LIMIT 1
        """, {'bid': bid, 'cid': cid})
        self.log_last_query(cur)
        if cur.fetchone():
            if only_check:
                return False, 'chunks_counters_queue_rows'
            return

        if only_check:
            return True, ''

        return Chunk(
            bid=chunk.bid, cid=chunk.cid,
            start_key=chunk.start_key, end_key=chunk.end_key,
            updated_ts=chunk.updated_ts,
        )

    def merge_chunks(self, zero_chunk, prev_chunk, pgmetadb, meta_user=None, attempts=1, sleep_interval=0, fail_step=None):
        # fail_step (int) - emulate fail after specified commit. Used for testing
        tpc_prefix = "merge"
        logging.info("Merging (%s, %s) to (%s, %s)", zero_chunk.bid, zero_chunk.cid, prev_chunk.bid, prev_chunk.cid)

        connstring_kwargs = {'application_name': const.CHUNK_MERGER_APPLICATION_NAME}
        if meta_user:
            connstring_kwargs['user'] = meta_user
        metadb, bucket = pgmetadb.get_s3meta_rw_by_bucket(zero_chunk.bid, **connstring_kwargs)
        metadb.commit()
        if not metadb:
            logging.error("Bucket by bid %s not found", zero_chunk.bid)
            return False
        logging.debug(metadb)
        logging.debug("Found bucket name '%s'", bucket.name)
        metadb.begin_tpc("{0}_{1}_{2}".format(tpc_prefix, zero_chunk.bid, zero_chunk.cid))

        # We need to acquire advisory lock on bucket to prevent
        # deadlocks while concurrent splitting different chunks
        # of one bucket due to deffered exclusion constraint.
        metadb.xact_advisory_lock(zero_chunk.bid)

        zero_chunk_at_meta = metadb.get_chunk(zero_chunk.bid, zero_chunk.cid)
        prev_chunk_at_meta = metadb.get_chunk(prev_chunk.bid, prev_chunk.cid)
        if zero_chunk_at_meta.shard_id != prev_chunk_at_meta.shard_id:
            raise ScriptException('Attempt to merge chunks at different shards')
        if zero_chunk_at_meta.bid != prev_chunk_at_meta.bid:
            raise ScriptException('Attempt to merge chunks of different buckets')
        if zero_chunk_at_meta.start_key != prev_chunk_at_meta.end_key:
            raise ScriptException('Attempt to merge chunks with separated bounds')

        self.shard_id = zero_chunk_at_meta.shard_id

        dbs = [self, metadb]
        self.begin_tpc("{0}_{1}_{2}_{3}".format(tpc_prefix, zero_chunk.bid, zero_chunk.cid, prev_chunk.cid))
        cur = self.create_cursor(cursor_factory=NamedTupleCursor)
        try:
            cur.execute("""
                SELECT * FROM v1_code.merge_chunks(
                    %(bid)s, %(prev_cid)s, %(zero_cid)s
                )
            """, {
                'bid': zero_chunk.bid,
                'zero_cid': zero_chunk.cid,
                'prev_cid': prev_chunk.cid,
            })
        except psycopg2.OperationalError as e:
            if e.pgcode == psycopg2.errorcodes.LOCK_NOT_AVAILABLE:
                logging.warning('Skip because of lock timeout.')
                self.reconnect()
                metadb.reconnect()
                return False
            raise
        self.log_last_query(cur)
        prev_chunk.end_key = zero_chunk.end_key
        metadb.update_chunk_boundaries(prev_chunk)
        metadb.delete_chunk_counters(zero_chunk)
        metadb.delete_chunk(zero_chunk)
        try:
            complete_tpc(dbs, pgmetadb, attempts, sleep_interval, fail_step)
        except psycopg2.IntegrityError as e:
            logging.error('Skip because of IntegrityError: %s', e)
            self.reconnect()
            metadb.reconnect()
            return False
        logging.info('Done.')
        return True

    def get_bid_size_by_ts(self, bid, storage_class, target_ts):
        cur = self.create_cursor()
        cur.execute("""
          WITH counters AS (
              SELECT
                  coalesce(sum(simple_objects_size), 0) +
                  coalesce(sum(multipart_objects_size), 0) +
                  coalesce(sum(objects_parts_size), 0)
                  AS size
                FROM s3.chunks_counters
                WHERE bid=%(bid)s AND coalesce(storage_class, 0) = %(storage_class)s
            ), billing AS (
              SELECT coalesce(sum(size_change), 0) AS usage_size
                FROM s3.buckets_usage
                WHERE bid=%(bid)s AND start_ts >= %(target_ts)s AND storage_class = %(storage_class)s
            )
          SELECT (counters.size - billing.usage_size)::bigint FROM counters, billing
        """, {'bid': bid, 'storage_class': storage_class, 'target_ts': target_ts})
        return cur.fetchone()[0]

    def insert_buckets_usage_diff(self, chunk, storage_class, size_change):
        cur = self.create_cursor()
        cur.execute("""
        INSERT INTO s3.buckets_usage AS c (
              bid,
              storage_class,
              size_change,
              byte_secs,
              start_ts,
              end_ts
           )
           VALUES (
                %(bid)s,
                %(storage_class)s,
                %(size_change)s,
                0,
                date_trunc('hour', current_timestamp),
                date_trunc('hour', current_timestamp) + '1 hour'::interval
            )
            ON CONFLICT (bid, start_ts, coalesce(storage_class, 0)) DO UPDATE
                SET size_change = EXCLUDED.size_change + c.size_change
            RETURNING bid
        """, {
            'bid': chunk.bid,
            'cid': chunk.cid,
            'storage_class': storage_class,
            'size_change': size_change,
        })
