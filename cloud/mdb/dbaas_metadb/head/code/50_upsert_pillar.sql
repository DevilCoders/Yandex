CREATE OR REPLACE FUNCTION code.upsert_pillar(
    i_cid   text,
    i_rev   bigint,
    i_value jsonb,
    i_key   code.pillar_key
) RETURNS void AS $$
BEGIN
    PERFORM code.check_pillar_key(i_key);
    CASE
        WHEN i_key.cid IS NOT NULL THEN
            INSERT INTO dbaas.pillar
                   (cid,
                    value)
            VALUES ((i_key).cid,
                    i_value)
            ON CONFLICT (cid) WHERE cid IS NOT NULL
               DO UPDATE
                 SET value = excluded.value;
        WHEN i_key.subcid IS NOT NULL THEN
            INSERT INTO dbaas.pillar
                   (subcid,
                    value)
            VALUES ((i_key).subcid,
                    i_value)
            ON CONFLICT (subcid) WHERE subcid IS NOT NULL
               DO UPDATE
                 SET value = excluded.value;
        WHEN i_key.shard_id IS NOT NULL THEN
            INSERT INTO dbaas.pillar
                   (shard_id,
                    value)
            VALUES ((i_key).shard_id,
                    i_value)
            ON CONFLICT (shard_id) WHERE shard_id IS NOT NULL
               DO UPDATE
                 SET value = excluded.value;
        WHEN i_key.fqdn IS NOT NULL THEN
            INSERT INTO dbaas.pillar
                   (fqdn,
                    value)
            VALUES ((i_key).fqdn,
                    i_value)
            ON CONFLICT (fqdn) WHERE fqdn IS NOT NULL
               DO UPDATE
                 SET value = excluded.value;
    END CASE;

    PERFORM code.update_cluster_change(
        i_cid,
        i_rev,
        jsonb_build_object(
            'upsert_pillar',
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
