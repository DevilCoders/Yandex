CREATE OR REPLACE FUNCTION code.add_alert_group(
    i_ag_id                 TEXT,
    i_cid                   TEXT,
    i_monitoring_folder_id  TEXT,
    i_managed               BOOLEAN,
    i_rev                   BIGINT

) RETURNS void AS $$
BEGIN
	INSERT INTO dbaas.alert_group (
		alert_group_id,
		cid,
		managed,
		monitoring_folder_id,
		status
    )
    VALUES (
		i_ag_id,
		i_cid,
		i_managed,
		i_monitoring_folder_id,
		'CREATING'
	);

	PERFORM code.update_cluster_change(
		i_cid,
		i_rev,
		jsonb_build_object(
			'add_alert_group',
			jsonb_build_object(
				'cid', i_cid,
				'alert_group', i_ag_id
			)
		)
	);
END;
$$ LANGUAGE plpgsql;
