CREATE MATERIALIZED VIEW s3.shard_disbalance_stat (
    shard_id,
    diff,
    k
) AS WITH all_shards AS (
      SELECT
        count(distinct shard_id) AS shards,
        sum(simple_objects_count) + sum(multipart_objects_count) AS objects
      FROM s3.chunks JOIN s3.chunks_counters USING (bid, cid)
  ),
  shards AS (
      SELECT
        shard_id,
        sum(simple_objects_count) + sum(multipart_objects_count) AS objects
      FROM s3.chunks join s3.chunks_counters USING (bid, cid)
      GROUP BY shard_id
  ),
  shard_diffs AS (
      SELECT
          shards.shard_id,
          all_shards.objects / all_shards.shards - shards.objects AS diff
        FROM shards, all_shards
        WHERE all_shards.shards > 0
  ),
  diffs AS (
    SELECT min(diff) as min_, max(diff) AS max_ FROM shard_diffs
  )
  SELECT shard_id, diff, diff / (max_ - min_) AS k
    FROM shard_diffs, diffs
    WHERE max_ > min_;

CREATE UNIQUE INDEX ON s3.shard_disbalance_stat (shard_id);
