CREATE OR REPLACE FUNCTION code.create_placement_group(
    i_cid                 text,
    i_rev                 bigint
) RETURNS bigint AS $$
DECLARE v_pg_id bigint;
BEGIN
    INSERT INTO dbaas.placement_groups as pg (
        cid, status
    )
    VALUES (
        i_cid, 'DESCRIBED'::dbaas.placement_group_status
    )
    RETURNING pg.pg_id INTO v_pg_id;

    PERFORM code.update_cluster_change(
        i_cid,
        i_rev,
        jsonb_build_object(
            'create_placement_group',
             jsonb_build_object(
                'cid', i_cid
            )
        )
    );

    RETURN v_pg_id;
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION code.create_placement_group(
    i_cid                 text,
    i_rev                 bigint,
    i_fqdn                text,
    i_local_id            bigint
) RETURNS bigint AS $$
DECLARE v_pg_id bigint;
BEGIN
    INSERT INTO dbaas.placement_groups as pg (
        cid, status, local_id, fqdn
    )
    VALUES (
        i_cid, 'DESCRIBED'::dbaas.placement_group_status, i_local_id, i_fqdn
    )
    RETURNING pg.pg_id INTO v_pg_id;

    PERFORM code.update_cluster_change(
        i_cid,
        i_rev,
        jsonb_build_object(
            'create_placement_group',
             jsonb_build_object(
                'cid', i_cid,
                'fqdn', i_fqdn,
                'local_id', i_local_id
            )
        )
    );

    RETURN v_pg_id;
END;
$$ LANGUAGE plpgsql;
