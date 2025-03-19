CREATE OR REPLACE FUNCTION v1_code.get_chunk_to_move(
    i_min_objects_diff bigint,
    i_min_objects bigint,
    i_max_objects bigint,
    i_created_delay interval DEFAULT '1 hour'::interval
)
RETURNS TABLE (
    source_shard int,
    dest_shard int,
    bid uuid,
    cid bigint
)
LANGUAGE plpgsql STABLE AS $$
DECLARE
    v_min bigint;
    v_max bigint;
    v_dest_shard int;
    v_source_shard int;
BEGIN
    /*
     * Find shards with biggest and smallest count of objects
     */
    SELECT shard_id,
        simple_objects_count + multipart_objects_count
    INTO v_source_shard, v_max
    FROM s3.shard_stat
    ORDER BY 2 DESC, chunks_count DESC, buckets_count DESC
    LIMIT 1;

    SELECT shard_id,
        simple_objects_count + multipart_objects_count
    INTO v_dest_shard, v_min
    FROM s3.shard_stat
    WHERE new_chunks_allowed
    ORDER BY 2, chunks_count, buckets_count
    LIMIT 1;

    IF v_max - v_min < i_min_objects_diff
        OR v_dest_shard IS NOT DISTINCT FROM v_source_shard THEN
        RETURN;
    END IF;

    RETURN QUERY
        SELECT v_source_shard, v_dest_shard, c.bid, c.cid
            FROM s3.chunks c JOIN s3.chunks_counters cc USING (bid, cid)
            WHERE c.shard_id = v_source_shard
                AND cc.simple_objects_count + cc.multipart_objects_count
                    BETWEEN i_min_objects AND i_max_objects
                -- Don't touch chunks with boundaries (NULL;NULL)
                AND coalesce(c.start_key, c.end_key) IS NOT NULL
                -- Delay from split
                AND c.created < current_timestamp - i_created_delay
            ORDER BY
                -- Find bucket with biggest count of objects in v_source_shard shard
                sum(simple_objects_count + multipart_objects_count)
                    OVER (PARTITION BY cc.bid) DESC,
                bid,
                -- Move last or first chunk dependently on v_dest_shard and v_source_shard
                CASE WHEN v_dest_shard > v_source_shard THEN c.start_key END DESC NULLS LAST,
                CASE WHEN v_dest_shard <= v_source_shard THEN c.start_key END ASC NULLS FIRST
            LIMIT 1;

    RETURN;
END
$$;
