CREATE OR REPLACE FUNCTION code.update_pillar(
    i_cid   text,
    i_rev   bigint,
    i_key   code.pillar_key,
    i_value jsonb
) RETURNS void AS $$
DECLARE
BEGIN
    PERFORM code.check_pillar_key(i_key);
    UPDATE dbaas.pillar
       SET value = i_value
     WHERE ((i_key).cid IS NULL OR (cid = (i_key).cid AND cid IS NOT NULL))
       AND ((i_key).subcid IS NULL OR (subcid = (i_key).subcid AND subcid IS NOT NULL))
       AND ((i_key).shard_id IS NULL OR (shard_id = (i_key).shard_id AND shard_id IS NOT NULL))
       AND ((i_key).fqdn IS NULL OR (fqdn = (i_key).fqdn AND fqdn IS NOT NULL));

    PERFORM code.update_cluster_change(
        i_cid,
        i_rev,
        jsonb_build_object(
            'update_pillar',
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
