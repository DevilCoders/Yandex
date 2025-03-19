DROP MATERIALIZED VIEW s3.shard_stat;

CREATE MATERIALIZED VIEW s3.shard_stat (
    shard_id,
    new_chunks_allowed,
    buckets_count,
    chunks_count,
    simple_objects_count,
    multipart_objects_count,
    deleted_objects_count,
    objects_parts_count,
    deleted_objects_parts_count
) AS SELECT
        s.shard_id,
        bool_and(s.new_chunks_allowed),
        count(distinct(bid)),
        count(c.cid),
        coalesce(sum(cc.simple_objects_count), 0)::bigint,
        coalesce(sum(cc.multipart_objects_count), 0)::bigint,
        coalesce(sum(cc.deleted_objects_count), 0)::bigint,
        coalesce(sum(cc.objects_parts_count), 0)::bigint,
        coalesce(sum(cc.deleted_objects_parts_count), 0)::bigint
    FROM s3.parts s
        LEFT JOIN s3.chunks c USING (shard_id)
        LEFT JOIN s3.chunks_counters cc USING (bid, cid)
    GROUP BY s.shard_id;

CREATE UNIQUE INDEX ON s3.shard_stat (shard_id);
