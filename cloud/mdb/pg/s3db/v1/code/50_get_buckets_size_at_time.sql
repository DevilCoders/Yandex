/*
 * Returns exact bucket size on shard (sum of chunk counters) at i_target_ts given that:
 *   s3.chunks.updated_ts < i_target_ts < max(s3.chunks_counters_queue.created_ts) < now()
 * This condition will be satisfied if we stop update_chunks_counters shortly before i_target_ts.
 */
CREATE OR REPLACE FUNCTION v1_code.get_buckets_size_at_time(
    i_target_ts timestamp with time zone,
    i_bid uuid DEFAULT NULL
) RETURNS TABLE (
  bid uuid,
  storage_class int,
  size bigint
)
LANGUAGE sql STABLE AS $$
    WITH base_data AS (
        SELECT
              bid,
              storage_class,
              sum(simple_objects_size) + sum(multipart_objects_size) + sum(objects_parts_size) AS bid_size
            FROM s3.chunks_counters
          WHERE (i_bid is NULL OR bid = i_bid)
          GROUP BY bid, storage_class
        ),
    counters_data AS (
        SELECT
              bid,
              coalesce(storage_class, 0) as storage_class,
              sum(simple_objects_size_change) + sum(multipart_objects_size_change) + sum(objects_parts_size_change) AS bid_size
            FROM s3.chunks_counters_queue
          WHERE (i_bid is NULL OR bid = i_bid) AND (created_ts < i_target_ts)
          GROUP BY bid, coalesce(storage_class, 0)
        ),
    all_data AS (
        SELECT * FROM base_data
        UNION ALL
        SELECT * FROM counters_data
        )
    SELECT bid, storage_class, sum(bid_size)::bigint FROM all_data GROUP BY bid, storage_class
$$;
