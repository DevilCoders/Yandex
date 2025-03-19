CREATE OR REPLACE FUNCTION code.update_disk(
    i_cid text,
	i_fqdn text,
	i_mount_point text,
	i_host_disk_id text,
	i_disk_id text,
	i_local_id bigint,
	i_status dbaas.disk_status,
	i_rev bigint
	) RETURNS void AS $$
DECLARE v_pg_id bigint;
BEGIN
    SELECT pg_id INTO v_pg_id
    FROM dbaas.disk_placement_groups pg
    WHERE pg.cid = i_cid
      AND pg.local_id = i_local_id;

    UPDATE dbaas.disks
    SET disk_id = i_disk_id,
        host_disk_id = i_host_disk_id,
        status = i_status
    WHERE fqdn = i_fqdn
      AND mount_point = i_mount_point
      AND pg_id = v_pg_id;

    PERFORM code.update_cluster_change(
            i_cid,
            i_rev,
            jsonb_build_object(
                    'update_disk',
                    jsonb_build_object(
                            'fqdn', i_fqdn,
                            'disk_id', i_disk_id
                        )
                )
        );

END;
$$ LANGUAGE plpgsql;
