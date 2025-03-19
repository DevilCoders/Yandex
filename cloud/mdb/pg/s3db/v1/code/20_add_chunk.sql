CREATE OR REPLACE FUNCTION v1_code.add_chunk(
    i_bid uuid,
    i_cid bigint,
    i_start_key text DEFAULT NULL,
    i_end_key text DEFAULT NULL
) RETURNS v1_code.chunk
LANGUAGE sql AS $function$
    INSERT INTO s3.chunks (
        bid, cid, start_key, end_key
    )
    VALUES (
        i_bid, i_cid, i_start_key, i_end_key
    )
    RETURNING bid, cid,
        /* created */ NULL::timestamptz,
        /* read_only */ NULL::boolean,
        start_key, end_key,
        /* shard_id */ NULL::int;
$function$;
