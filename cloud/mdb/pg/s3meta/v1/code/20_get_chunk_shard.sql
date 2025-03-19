CREATE OR REPLACE FUNCTION v1_code.get_chunk_shard(
    i_bucket_name text,
    i_bid uuid,
    i_cid bigint
) RETURNS int
LANGUAGE plpgsql STABLE AS $function$
DECLARE
    v_shard_id int;
BEGIN

    SELECT shard_id INTO v_shard_id FROM s3.chunks
        WHERE bid = i_bid AND cid = i_cid;

    IF NOT FOUND THEN
        RAISE EXCEPTION 'No such chunk'
            USING ERRCODE = 'S3X01';
    END IF;

    RETURN v_shard_id;
END
$function$;
