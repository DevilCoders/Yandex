/*
 * DEPRECATED
 * Returns buckets statistics per service.
 *
 * Args:
 * - i_service_id:
 *     Id of the service. If NULL passed return all buckets statistics.
 *
 */
CREATE OR REPLACE FUNCTION v1_code.get_service_buckets_stats(
    i_service_id bigint
) RETURNS SETOF v1_code.bucket_stat
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
    FROM s3.bucket_stat
    WHERE (i_service_id IS NULL OR service_id = i_service_id);
$function$;
