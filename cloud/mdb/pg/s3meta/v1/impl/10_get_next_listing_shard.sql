CREATE OR REPLACE FUNCTION v1_impl.get_next_listing_shard(
    i_bid uuid,
    i_shard_id int
)
RETURNS TABLE (
    shard_id int,
    min_key text
)
LANGUAGE sql STABLE
ROWS 1
AS $function$
    SELECT shard_id, coalesce(start_key, '')
        FROM s3.chunks
        WHERE bid = i_bid
            AND shard_id > i_shard_id
    ORDER BY shard_id, coalesce(start_key, '') ASC
    LIMIT 1;
$function$;
