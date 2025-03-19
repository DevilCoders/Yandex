CREATE OR REPLACE FUNCTION v1_impl.refresh_bucket_storage_class_stat()
RETURNS void
LANGUAGE sql
SECURITY DEFINER AS $function$
    REFRESH MATERIALIZED VIEW CONCURRENTLY s3.bucket_storage_class_stat;
$function$;
