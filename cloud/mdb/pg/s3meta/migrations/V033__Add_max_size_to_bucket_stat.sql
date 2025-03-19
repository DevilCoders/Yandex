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
    updated_ts,
    max_size
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
       current_timestamp,
       b.max_size
     FROM s3.buckets b
       LEFT JOIN s3.chunks_counters c USING (bid)
     GROUP BY b.name, b.bid, b.service_id;

CREATE UNIQUE INDEX ON s3.bucket_stat (name);
CREATE INDEX ON s3.bucket_stat(service_id);

