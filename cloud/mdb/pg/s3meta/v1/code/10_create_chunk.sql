CREATE OR REPLACE FUNCTION v1_code.create_chunk(
    i_bid uuid,
    i_start_key text DEFAULT NULL,
    i_end_key text DEFAULT NULL,
    i_shard_id int DEFAULT NULL
) RETURNS v1_code.chunk
LANGUAGE plpgsql VOLATILE AS $function$
DECLARE
    v_shard_id int;
    v_chunk v1_code.chunk;
BEGIN
    /*
     * Find optimal shard with lowest count of objects
     * if i_shard_id wasn't specified
     */
    v_shard_id := i_shard_id;
    IF v_shard_id IS NULL THEN
        SELECT shard_id INTO STRICT v_shard_id
            FROM s3.shard_stat
            WHERE new_chunks_allowed
            ORDER BY simple_objects_count + multipart_objects_count,
                chunks_count, buckets_count, shard_id
            LIMIT 1;
    END IF;

    INSERT INTO s3.chunks (bid, start_key, end_key, shard_id)
        VALUES (i_bid, i_start_key, i_end_key, v_shard_id)
    RETURNING * INTO v_chunk;

    INSERT INTO s3.chunks_create_queue (bid, cid, created, start_key, end_key, shard_id)
        VALUES (i_bid, v_chunk.cid, v_chunk.created, i_start_key, i_end_key, v_shard_id);

    RETURN v_chunk;
END;
$function$;
