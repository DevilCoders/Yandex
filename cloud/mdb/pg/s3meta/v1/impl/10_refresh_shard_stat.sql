CREATE OR REPLACE FUNCTION v1_impl.refresh_shard_stat()
RETURNS void
LANGUAGE sql
SECURITY DEFINER AS $function$
    REFRESH MATERIALIZED VIEW CONCURRENTLY s3.shard_stat;
$function$;
