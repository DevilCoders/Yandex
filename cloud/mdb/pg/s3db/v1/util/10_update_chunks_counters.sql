CREATE OR REPLACE FUNCTION util.update_chunks_counters(
    i_limit integer DEFAULT 1000
) RETURNS TABLE (
    rows bigint,
    chunks bigint
)
LANGUAGE sql
ROWS 1 AS $$
    WITH batch AS (
        SELECT * FROM s3.chunks_counters_queue
            ORDER BY id
            LIMIT i_limit
            FOR UPDATE SKIP LOCKED
        ),
    deleted_rows AS (
        DELETE FROM s3.chunks_counters_queue q
          USING batch WHERE q.id = batch.id
          RETURNING q.*
        ),
    summarized_change AS (
        SELECT bid, cid,
               sum(simple_objects_count_change) AS simple_objects_count,
               sum(simple_objects_size_change) AS simple_objects_size,
               sum(multipart_objects_count_change) AS multipart_objects_count,
               sum(multipart_objects_size_change) AS multipart_objects_size,
               sum(objects_parts_count_change) AS objects_parts_count,
               sum(objects_parts_size_change) AS objects_parts_size,
               sum(deleted_objects_count_change) AS deleted_objects_count,
               sum(deleted_objects_size_change) AS deleted_objects_size,
               sum(active_multipart_count_change) AS active_multipart_count,
               coalesce(storage_class, 0) AS storage_class
            FROM deleted_rows
          GROUP BY bid, cid, coalesce(storage_class, 0)
          HAVING
            sum(simple_objects_count_change) != 0
            OR sum(simple_objects_size_change) != 0
            OR sum(multipart_objects_count_change) != 0
            OR sum(multipart_objects_size_change) != 0
            OR sum(objects_parts_count_change) != 0
            OR sum(objects_parts_size_change) != 0
            OR sum(deleted_objects_count_change) != 0
            OR sum(deleted_objects_size_change) != 0
            OR sum(active_multipart_count_change) != 0
        ),
    summarized_change_bytesecs AS (
        SELECT bid,
               coalesce(storage_class, 0) AS storage_class,
               sum(simple_objects_size_change + multipart_objects_size_change + objects_parts_size_change) AS size_change,
               sum((simple_objects_size_change + multipart_objects_size_change + objects_parts_size_change) *
                   (ROUND(extract(epoch FROM date_trunc('hour', created_ts) + '1 hour'::interval - created_ts))::integer)) AS byte_secs,
               date_trunc('hour', created_ts) AS start_ts,
               date_trunc('hour', created_ts) + '1 hour'::interval AS end_ts
          FROM deleted_rows
          WHERE simple_objects_size_change != 0
            OR multipart_objects_size_change != 0
            OR objects_parts_size_change != 0
          GROUP BY bid, coalesce(storage_class, 0), start_ts, end_ts
        ),
    update_counters AS (
        INSERT INTO s3.chunks_counters AS d (
              bid,
              cid,
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
              updated_ts
          )
          SELECT
              bid,
              cid,
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
              current_timestamp
            FROM summarized_change
            ON CONFLICT (bid, cid, storage_class) DO UPDATE
                SET
                  simple_objects_count = EXCLUDED.simple_objects_count + d.simple_objects_count,
                  simple_objects_size = EXCLUDED.simple_objects_size + d.simple_objects_size,
                  multipart_objects_count = EXCLUDED.multipart_objects_count + d.multipart_objects_count,
                  multipart_objects_size = EXCLUDED.multipart_objects_size + d.multipart_objects_size,
                  objects_parts_count = EXCLUDED.objects_parts_count + d.objects_parts_count,
                  objects_parts_size = EXCLUDED.objects_parts_size + d.objects_parts_size,
                  deleted_objects_count = EXCLUDED.deleted_objects_count + d.deleted_objects_count,
                  deleted_objects_size = EXCLUDED.deleted_objects_size + d.deleted_objects_size,
                  active_multipart_count = EXCLUDED.active_multipart_count + d.active_multipart_count,
                  updated_ts = current_timestamp
    ),
    update_counters_bytesecs AS (
        INSERT INTO s3.buckets_usage AS c (
              bid,
              storage_class,
              size_change,
              byte_secs,
              start_ts,
              end_ts
          )
          SELECT
              bid,
              storage_class,
              size_change,
              byte_secs,
              start_ts,
              end_ts
            FROM summarized_change_bytesecs
            ON CONFLICT (bid, start_ts, coalesce(storage_class, 0)) DO UPDATE
                SET size_change = EXCLUDED.size_change + c.size_change,
                    byte_secs = EXCLUDED.byte_secs + c.byte_secs
            RETURNING bid
    )
    SELECT count(1), count(distinct(cid)) FROM batch;
$$;
