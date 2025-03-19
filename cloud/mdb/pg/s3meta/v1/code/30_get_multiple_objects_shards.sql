CREATE OR REPLACE FUNCTION v1_code.get_multiple_objects_shards(
    i_bucket_name text,
    i_multiple_objects text[]
) RETURNS SETOF v1_code.object_shard
LANGUAGE sql STABLE AS $function$
    SELECT name, v1_code.get_object_shard(i_bucket_name, name) as shard_id
    FROM unnest(i_multiple_objects) as f(name);
$function$;
