/*
 * DEPRECATED
 * Returns statistics of all buckets
 *
 */
CREATE OR REPLACE FUNCTION v1_code.get_buckets_stats()
RETURNS SETOF v1_code.bucket_stat
LANGUAGE sql STABLE AS
$function$
    SELECT
        bid,
        name,
        service_id,
        CAST(0 AS INT) as storage_class,
        chunks_count,
        simple_objects_count,
        simple_objects_size,
        multipart_objects_count,
        multipart_objects_size,
        objects_parts_count,
        objects_parts_size,
        updated_ts,
        max_size
    FROM s3.bucket_stat;
$function$;
