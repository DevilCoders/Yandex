CREATE OR REPLACE FUNCTION v1_code.merge_chunks(
    i_bid uuid,
    i_prev_cid bigint,  /* for updating */
    i_next_cid bigint   /* for deleting */
) RETURNS void
LANGUAGE plpgsql VOLATILE AS $function$
DECLARE
    v_chunks_counters_changes v1_code.chunk_counters[];
    v_end_key text;
BEGIN
    /*
     * Block chunk and chunks_counters FOR UPDATE to prevent any operations
     * with objects because we need to calculate counters
     * correctly for new chunk
     */
    PERFORM
      FROM s3.chunks WHERE bid = i_bid AND cid IN (i_next_cid, i_prev_cid)
    FOR UPDATE;

    PERFORM
      FROM s3.chunks_counters WHERE bid = i_bid AND cid IN (i_next_cid, i_prev_cid)
    FOR UPDATE;

    PERFORM FROM s3.chunks
      WHERE bid = i_bid
        AND cid = i_prev_cid
        AND end_key = (
          SELECT start_key
          FROM s3.chunks where bid = i_bid AND cid = i_next_cid
        );
    IF NOT FOUND THEN
      RAISE EXCEPTION 'Chunks (%, %) and (%, %) do not follow each other', i_bid, i_prev_cid, i_bid, i_next_cid;
    END IF;

    /* Update chunks counters */
    WITH deleted_counters AS (
      DELETE FROM s3.chunks_counters
        WHERE bid = i_bid
        AND cid = i_next_cid
        RETURNING *
    ) SELECT v1_code.chunks_counters_queue_push((i_bid,
      i_prev_cid,
      simple_objects_count,
      simple_objects_size,
      multipart_objects_count,
      multipart_objects_size,
      objects_parts_count,
      objects_parts_size,
      deleted_objects_count,
      deleted_objects_size,
      active_multipart_count,
      storage_class)::v1_code.chunk_counters) FROM deleted_counters
    INTO v_chunks_counters_changes;

    UPDATE s3.chunks_counters_queue
      SET cid = i_prev_cid
      WHERE bid = i_bid AND cid = i_next_cid;

    /* Delete old chunk and update new */
    SELECT end_key
      FROM s3.chunks
      WHERE bid = i_bid AND cid = i_next_cid
      INTO v_end_key;

    DELETE FROM s3.chunks
      WHERE bid = i_bid
      AND cid = i_next_cid;

    UPDATE s3.chunks SET end_key = v_end_key
      WHERE bid = i_bid AND cid = i_prev_cid;

END;
$function$;
