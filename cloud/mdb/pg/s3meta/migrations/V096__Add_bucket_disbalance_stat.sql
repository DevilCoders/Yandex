CREATE MATERIALIZED VIEW s3.bucket_disbalance_stat (
    bid,
    shard_id,
    objects,
    avg_objects,
    perc_diff
) AS WITH bid_stat AS (
      SELECT chunks.bid,
          chunks.shard_id,
          sum(chunks_counters.simple_objects_count + chunks_counters.multipart_objects_count) AS objects
        FROM s3.chunks
        JOIN s3.chunks_counters USING (bid, cid)
        GROUP BY chunks.bid, chunks.shard_id
  ),
  bid_avg AS (
      SELECT bid,
          avg(objects)::bigint AS avg_objects
        FROM bid_stat
        GROUP BY bid
  )
  SELECT bid_stat.bid,
      bid_stat.shard_id,
       objects,
       avg_objects,
       100 * abs(objects - avg_objects) / GREATEST(avg_objects, 1) AS perc_diff
    FROM bid_stat LEFT JOIN bid_avg USING (bid);

CREATE UNIQUE INDEX ON s3.bucket_disbalance_stat (bid, shard_id);
