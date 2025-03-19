CREATE MATERIALIZED VIEW s3.bucket_stat (
    bid,
    name,
    service_id,
    chunks_count,
    simple_objects_count,
    simple_objects_size,
    multipart_objects_count,
    multipart_objects_size,
    deleted_objects_count,
    deleted_objects_size,
    objects_parts_count,
    objects_parts_size,
    deleted_objects_parts_count,
    deleted_objects_parts_size,
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
       coalesce(sum(c.deleted_objects_count), 0)::bigint,
       coalesce(sum(c.deleted_objects_size), 0)::bigint,
       coalesce(sum(c.objects_parts_count), 0)::bigint,
       coalesce(sum(c.objects_parts_size), 0)::bigint,
       coalesce(sum(c.deleted_objects_parts_count), 0)::bigint,
       coalesce(sum(c.deleted_objects_parts_size), 0)::bigint,
       current_timestamp
     FROM s3.buckets b
       LEFT JOIN s3.chunks_counters c USING (bid)
     GROUP BY b.name, b.bid, b.service_id;

CREATE UNIQUE INDEX ON s3.bucket_stat (name);
CREATE INDEX ON s3.bucket_stat(service_id);
