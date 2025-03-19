CREATE OR REPLACE FUNCTION code.delete_alert_group(
	i_alert_group_id               TEXT,
	i_cid                          TEXT,
	i_rev                          bigint,
	i_force_managed_group_deletion BOOLEAN DEFAULT FALSE
) RETURNS void AS $$
BEGIN

	IF NOT i_force_managed_group_deletion AND (SELECT managed from dbaas.alert_group WHERE alert_group_id = i_alert_group_id) THEN
		RAISE EXCEPTION 'Deletion of managed alert group % is prohibited', i_alert_group_id USING ERRCODE = '23514';
	END IF;
	UPDATE dbaas.alert_group SET status = 'DELETING' WHERE alert_group_id = i_alert_group_id;

	PERFORM code.update_cluster_change(
		i_cid,
		i_rev,
		jsonb_build_object(
			'delete_alert_group',
			 jsonb_build_object(
				'cid', i_cid,
				'ag_id', i_alert_group_id
			)
		)
	);
END;
$$ LANGUAGE plpgsql;
