CREATE OR REPLACE FUNCTION code.add_shard(
    i_subcid    text,
    i_shard_id  text,
    i_name      text,
    i_cid       text,
    i_rev       bigint
) RETURNS TABLE (subcid text, shard_id text, name text) AS $$
BEGIN
    INSERT INTO dbaas.shards (
        subcid, shard_id,
        name
    ) VALUES (
        i_subcid, i_shard_id,
        i_name
    );

    PERFORM code.update_cluster_change(
        i_cid,
        i_rev,
        jsonb_build_object(
            'add_shard',
             jsonb_build_object(
                'subcid', i_subcid,
                'shard_id', i_shard_id,
                'name', i_name
            )
        )
    );

    RETURN QUERY SELECT i_subcid, i_shard_id, i_name;
END;
$$ LANGUAGE plpgsql;
