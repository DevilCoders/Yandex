/*
 * Returns counters for specific chunk of all chunks of bucket.
 *
 * Args:
 * - i_bid:
 *     ID of the bucket.
 * - i_cid:
 *     Chunk ID.
 *
 * Returns:
 *   A set of ``code.chunk_counters`` instances that represents the bucket
 *   or specific chunk.
 */
CREATE OR REPLACE FUNCTION v1_code.get_chunks_counters(
    i_bid uuid,
    i_cid bigint DEFAULT NULL
) RETURNS SETOF v1_code.chunk_counters
LANGUAGE sql STABLE AS $$
    WITH chunks_data AS (
        SELECT bid, cid,
               simple_objects_count, simple_objects_size,
               multipart_objects_count, multipart_objects_size,
               objects_parts_count, objects_parts_size,
               deleted_objects_count, deleted_objects_size,
               active_multipart_count
            FROM s3.chunks_counters
          WHERE bid = i_bid
            AND (i_cid IS NULL OR cid = i_cid)
        ),
    counters_data AS (
        SELECT bid, cid,
               sum(simple_objects_count_change) AS simple_objects_count,
               sum(simple_objects_size_change) AS simple_objects_size,
               sum(multipart_objects_count_change) AS multipart_objects_count,
               sum(multipart_objects_size_change) AS multipart_objects_size,
               sum(objects_parts_count_change) AS objects_parts_count,
               sum(objects_parts_size_change) AS objects_parts_size,
               sum(deleted_objects_count_change) AS deleted_objects_count,
               sum(deleted_objects_size_change) AS deleted_objects_size,
               sum(active_multipart_count_change) AS active_multipart_count
            FROM s3.chunks_counters_queue
          WHERE bid = i_bid
            AND (i_cid IS NULL OR cid = i_cid)
          GROUP BY bid, cid
        ),
    all_data AS (
        SELECT * FROM chunks_data
        UNION ALL
        SELECT * FROM counters_data
        )
    SELECT bid, cid,
           sum(simple_objects_count)::bigint AS simple_objects_count,
           sum(simple_objects_size)::bigint AS simple_objects_size,
           sum(multipart_objects_count)::bigint AS multipart_objects_count,
           sum(multipart_objects_size)::bigint AS multipart_objects_size,
           sum(objects_parts_count)::bigint AS objects_parts_count,
           sum(objects_parts_size)::bigint AS objects_parts_size,
           sum(deleted_objects_count)::bigint AS deleted_objects_count,
           sum(deleted_objects_size)::bigint AS deleted_objects_size,
           sum(active_multipart_count)::bigint AS active_multipart_count,
           NULL::int as storage_class
        FROM all_data
      GROUP BY bid, cid;
$$;
