CREATE OR REPLACE FUNCTION code.update_placement_group(
	i_cid text,
	i_placement_group_id text,
	i_rev bigint,
	i_status dbaas.placement_group_status
	) RETURNS void AS
$$
BEGIN
    UPDATE dbaas.placement_groups
    SET placement_group_id = i_placement_group_id,
        status             = i_status
    WHERE cid = i_cid;

    PERFORM code.update_cluster_change(
            i_cid,
            i_rev,
            jsonb_build_object(
                    'update_placement_group',
                    jsonb_build_object(
                            'cid', i_cid,
                            'placement_group_id', i_placement_group_id
                        )
                )
        );

END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION code.update_placement_group(
	i_cid text,
	i_placement_group_id text,
	i_rev bigint,
	i_status dbaas.placement_group_status,
        i_fqdn text
	) RETURNS void AS
$$
BEGIN
    UPDATE dbaas.placement_groups
    SET placement_group_id = i_placement_group_id,
        status             = i_status
    WHERE cid = i_cid AND fqdn = i_fqdn;

    PERFORM code.update_cluster_change(
            i_cid,
            i_rev,
            jsonb_build_object(
                    'update_placement_group',
                    jsonb_build_object(
                            'cid', i_cid,
                            'placement_group_id', i_placement_group_id,
                            'fqdn', i_fqdn
                        )
                )
        );

END;
$$ LANGUAGE plpgsql;
