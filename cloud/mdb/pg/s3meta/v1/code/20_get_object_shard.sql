CREATE OR REPLACE FUNCTION v1_code.get_object_shard(
    i_bucket_name text,
    i_name text,
    i_write boolean DEFAULT false
) RETURNS int
LANGUAGE sql STABLE AS $function$
    SELECT shard_id FROM v1_code.get_object_chunk(
        i_bucket_name, i_name, i_write
    );
$function$;
