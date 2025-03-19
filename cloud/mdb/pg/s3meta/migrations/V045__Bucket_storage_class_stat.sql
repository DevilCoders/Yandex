CREATE MATERIALIZED VIEW s3.bucket_storage_class_stat (
    bid,
    name,
    storage_class,
    size
) AS SELECT
      b.bid,
      b.name,
      storage_class,
      size
    FROM (
      SELECT bid, storage_class, sum(shard_size)::bigint AS size FROM (
        SELECT bid, storage_class, (base_size + sum(size_change)) AS shard_size
          FROM (
            SELECT s.bid, s.shard_id, s.storage_class, s.target_ts AS base_ts, u.change_ts, s.size AS base_size, u.size_change
              FROM (
                SELECT a.bid, a.shard_id, a.storage_class, a.target_ts, a.size
                  FROM s3.buckets_size a
                  JOIN (
                    SELECT bid, shard_id, storage_class, max(target_ts) AS target_ts
                      FROM s3.buckets_size
                      GROUP by bid, shard_id, storage_class
                  ) b
                  ON (a.bid = b.bid AND a.shard_id = b.shard_id AND a.storage_class = b.storage_class AND a.target_ts = b.target_ts)
              ) s
              JOIN (
                 SELECT bid, shard_id, storage_class, start_ts AS change_ts, size_change
                   FROM s3.buckets_usage
              ) u ON (u.bid=s.bid AND u.shard_id = s.shard_id AND u.storage_class = s.storage_class AND u.change_ts >= s.target_ts)
          ) d
          GROUP BY bid, storage_class, base_size, shard_id
      ) z GROUP BY bid, storage_class
    ) r JOIN s3.buckets b ON (r.bid = b.bid);

CREATE UNIQUE INDEX ON s3.bucket_storage_class_stat (name, storage_class);
