CREATE OR REPLACE FUNCTION code.update_disk_placement_group(
	i_cid text,
	i_disk_placement_group_id text,
	i_local_id bigint,
	i_rev bigint,
	i_status dbaas.disk_placement_group_status
	) RETURNS void AS
$$
BEGIN
    UPDATE dbaas.disk_placement_groups
    SET disk_placement_group_id = i_disk_placement_group_id,
        status                  = i_status
    WHERE cid = i_cid
      AND local_id = i_local_id;

    PERFORM code.update_cluster_change(
            i_cid,
            i_rev,
            jsonb_build_object(
                    'update_placement_group',
                    jsonb_build_object(
                            'cid', i_cid,
                            'disk_placement_group_id', i_disk_placement_group_id
                        )
                )
        );

END;
$$ LANGUAGE plpgsql;
