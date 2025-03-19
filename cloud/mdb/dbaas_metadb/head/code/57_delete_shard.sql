CREATE OR REPLACE FUNCTION code.delete_shard(
    i_cid       text,
    i_shard_id  text,
    i_rev       bigint
) RETURNS void AS $$
BEGIN
    DELETE FROM dbaas.pillar
     WHERE shard_id = i_shard_id;

    DELETE FROM dbaas.shards
     WHERE shard_id = i_shard_id;

    PERFORM code.update_cluster_change(
        i_cid,
        i_rev,
        jsonb_build_object(
            'delete_shard',
             jsonb_build_object(
                'shard_id', i_shard_id
            )
        )
    );
END;
$$ LANGUAGE plpgsql;
