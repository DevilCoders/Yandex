DROP MATERIALIZED VIEW s3.bucket_storage_class_stat;
CREATE MATERIALIZED VIEW s3.bucket_storage_class_stat (
    bid,
    name,
    service_id,
    storage_class,
    chunks_count,
    simple_objects_count,
    simple_objects_size,
    multipart_objects_count,
    multipart_objects_size,
    objects_parts_count,
    objects_parts_size,
    deleted_objects_count,
    deleted_objects_size,
    active_multipart_count,
    updated_ts,
    max_size
) AS SELECT
       b.bid,
       b.name,
       b.service_id,
       c.storage_class,
       count(c.cid),
       coalesce(sum(c.simple_objects_count), 0)::bigint,
       coalesce(sum(c.simple_objects_size), 0)::bigint,
       coalesce(sum(c.multipart_objects_count), 0)::bigint,
       coalesce(sum(c.multipart_objects_size), 0)::bigint,
       coalesce(sum(c.objects_parts_count), 0)::bigint,
       coalesce(sum(c.objects_parts_size), 0)::bigint,
       coalesce(sum(c.deleted_objects_count), 0)::bigint,
       coalesce(sum(c.deleted_objects_size), 0)::bigint,
       coalesce(sum(c.active_multipart_count), 0)::bigint,
       coalesce(max(c.inserted_ts), b.updated_ts),
       b.max_size
     FROM s3.buckets b
     LEFT JOIN (
        SELECT * FROM s3.chunks_counters
          WHERE simple_objects_count >= 0 AND simple_objects_size >= 0
            AND multipart_objects_count >= 0 AND multipart_objects_size >= 0
            AND objects_parts_count >= 0 AND objects_parts_size >= 0
       ) c USING (bid)
     GROUP BY b.name, b.bid, b.service_id, c.storage_class;

CREATE UNIQUE INDEX ON s3.bucket_storage_class_stat(name, storage_class);
CREATE INDEX ON s3.bucket_storage_class_stat(service_id);
CREATE INDEX ON s3.bucket_storage_class_stat(bid);
