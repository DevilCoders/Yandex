CREATE OR REPLACE FUNCTION code.delete_alert_from_group(
	i_cid             TEXT,
	i_alert_group_id  TEXT,
	i_template_id     TEXT,
	i_rev             BIGINT
) RETURNS void AS $$
BEGIN

    UPDATE dbaas.alert SET status = 'DELETING' WHERE alert_group_id = i_alert_group_id AND template_id = i_template_id;

	PERFORM code.update_cluster_change(
		i_cid,
		i_rev,
		jsonb_build_object(
			'delete_alert_from_group',
			 jsonb_build_object(
				'cid', i_cid,
				'ag_id', i_alert_group_id,
				'template_id', i_template_id
			)
		)
	);
END;
$$ LANGUAGE plpgsql;
