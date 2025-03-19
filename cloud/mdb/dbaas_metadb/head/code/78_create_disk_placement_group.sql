CREATE OR REPLACE FUNCTION code.create_disk_placement_group(
    i_cid                 text,
    i_local_id            bigint,
    i_rev                 bigint
) RETURNS bigint AS $$
DECLARE v_pg_id bigint;
BEGIN
    INSERT INTO dbaas.disk_placement_groups as pg (
        cid, local_id, status
    )
    VALUES (
        i_cid, i_local_id, 'DESCRIBED'::dbaas.disk_placement_group_status
    )
    RETURNING pg.pg_id INTO v_pg_id;

    PERFORM code.update_cluster_change(
        i_cid,
        i_rev,
        jsonb_build_object(
            'create_placement_group',
             jsonb_build_object(
                'cid', i_cid,
                'local_id', i_local_id
            )
        )
    );

    RETURN v_pg_id;    
END;
$$ LANGUAGE plpgsql;
