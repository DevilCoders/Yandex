CREATE OR REPLACE FUNCTION code.create_disk(
    i_cid                 text,
    i_local_id            bigint,
    i_fqdn                text,
    i_mount_point         text,
    i_rev                 bigint
) RETURNS bigint AS $$
DECLARE v_d_id bigint;
DECLARE v_pg_id bigint;
BEGIN
    SELECT pg_id INTO v_pg_id
        FROM dbaas.disk_placement_groups pg
        WHERE pg.cid = i_cid AND pg.local_id = i_local_id;

    INSERT INTO dbaas.disks as d (
        pg_id, fqdn, mount_point, status, cid
    )
    VALUES (
        v_pg_id, i_fqdn, i_mount_point, 'DESCRIBED'::dbaas.disk_status, i_cid
    )
    RETURNING d.d_id INTO v_d_id;

    PERFORM code.update_cluster_change(
        i_cid,
        i_rev,
        jsonb_build_object(
            'create_disk',
             jsonb_build_object(
                'cid', i_cid,
                'local_id', i_local_id,
                'fqdn', i_fqdn,
                'mount_point', i_mount_point
            )
        )
    );

    RETURN v_d_id;    
END;
$$ LANGUAGE plpgsql;
