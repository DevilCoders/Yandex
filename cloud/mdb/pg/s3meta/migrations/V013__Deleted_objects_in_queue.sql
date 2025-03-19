DROP MATERIALIZED VIEW s3.shard_stat;
CREATE MATERIALIZED VIEW s3.shard_stat (
    shard_id,
    new_chunks_allowed,
    buckets_count,
    chunks_count,
    simple_objects_count,
    multipart_objects_count,
    objects_parts_count
) AS SELECT
        s.shard_id,
        bool_and(s.new_chunks_allowed),
        count(distinct(bid)),
        count(c.cid),
        coalesce(sum(cc.simple_objects_count), 0)::bigint,
        coalesce(sum(cc.multipart_objects_count), 0)::bigint,
        coalesce(sum(cc.objects_parts_count), 0)::bigint
    FROM s3.parts s
        LEFT JOIN s3.chunks c USING (shard_id)
        LEFT JOIN s3.chunks_counters cc USING (bid, cid)
    GROUP BY s.shard_id;
CREATE UNIQUE INDEX ON s3.shard_stat (shard_id);

DROP MATERIALIZED VIEW s3.bucket_stat;
CREATE MATERIALIZED VIEW s3.bucket_stat (
    bid,
    name,
    service_id,
    chunks_count,
    simple_objects_count,
    simple_objects_size,
    multipart_objects_count,
    multipart_objects_size,
    objects_parts_count,
    objects_parts_size,
    updated_ts
) AS SELECT
       b.bid,
       b.name,
       b.service_id,
       count(c.cid),
       coalesce(sum(c.simple_objects_count), 0)::bigint,
       coalesce(sum(c.simple_objects_size), 0)::bigint,
       coalesce(sum(c.multipart_objects_count), 0)::bigint,
       coalesce(sum(c.multipart_objects_size), 0)::bigint,
       coalesce(sum(c.objects_parts_count), 0)::bigint,
       coalesce(sum(c.objects_parts_size), 0)::bigint,
       current_timestamp
     FROM s3.buckets b
       LEFT JOIN s3.chunks_counters c USING (bid)
     GROUP BY b.name, b.bid, b.service_id;

CREATE UNIQUE INDEX ON s3.bucket_stat (name);
CREATE INDEX ON s3.bucket_stat(service_id);

ALTER TABLE s3.chunks_counters
    DROP COLUMN deleted_objects_count,
    DROP COLUMN deleted_objects_size,
    DROP COLUMN deleted_objects_parts_count,
    DROP COLUMN deleted_objects_parts_size;
