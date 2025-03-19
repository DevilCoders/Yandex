/*
 * Push chunk moving task to queue.
 */
CREATE OR REPLACE FUNCTION v1_code.chunk_move_queue_push(
    i_source_shard int,
    i_dest_shard int,
    i_bid uuid,
    i_cid bigint,
    priority int DEFAULT 0
) RETURNS void LANGUAGE plpgsql AS
$$
BEGIN
    INSERT INTO s3.chunks_move_queue (bid, cid, source_shard, dest_shard, priority)
    VALUES (i_bid, i_cid, i_source_shard, i_dest_shard, priority);
END;
$$;
