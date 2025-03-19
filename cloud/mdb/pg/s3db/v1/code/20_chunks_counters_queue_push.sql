CREATE OR REPLACE FUNCTION v1_code.chunks_counters_queue_push(
    i_chunk_counters_change v1_code.chunk_counters
) RETURNS void
LANGUAGE sql VOLATILE STRICT AS $function$
    INSERT INTO s3.chunks_counters_queue (
            bid, cid,
            simple_objects_count_change,
            simple_objects_size_change,
            multipart_objects_count_change,
            multipart_objects_size_change,
            objects_parts_count_change,
            objects_parts_size_change,
            deleted_objects_count_change,
            deleted_objects_size_change,
            active_multipart_count_change,
            storage_class)
        VALUES (i_chunk_counters_change.*);
$function$;
