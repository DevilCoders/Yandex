CREATE OR REPLACE FUNCTION code.add_pillar(
    i_cid   text,
    i_rev   bigint,
    i_key   code.pillar_key,
    i_value jsonb
) RETURNS void AS $$
BEGIN
    PERFORM code.check_pillar_key(i_key);
    INSERT INTO dbaas.pillar (
        cid, subcid,
        shard_id, fqdn,
        value
    ) VALUES (
        (i_key).cid, (i_key).subcid,
        (i_key).shard_id, (i_key).fqdn,
        i_value
    );

    PERFORM code.update_cluster_change(
        i_cid,
        i_rev,
        jsonb_build_object(
            'add_pillar',
            jsonb_build_object(
                'cid', (i_key).cid,
                'subcid', (i_key).subcid,
                'shard_id', (i_key).shard_id,
                'fqdn', (i_key).fqdn
            )
        )
    );
END;
$$ LANGUAGE plpgsql;
