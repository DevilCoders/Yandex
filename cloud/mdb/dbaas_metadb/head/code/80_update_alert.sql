CREATE OR REPLACE FUNCTION code.update_alert (
	i_cid                   TEXT,
	i_alert_group_id        TEXT,
	i_template_id           TEXT,
	i_notification_channels TEXT[],
	i_disabled              BOOLEAN,
	i_crit_threshold        NUMERIC,
	i_warn_threshold        NUMERIC,
	i_default_thresholds    BOOLEAN,
	i_rev                   BIGINT
) RETURNS VOID AS $$ 
BEGIN
	UPDATE
		dbaas.alert 
	SET
		critical_threshold = i_crit_threshold,
		warning_threshold = i_warn_threshold,
		notification_channels = i_notification_channels,
		disabled = i_disabled,
		default_thresholds = i_default_thresholds,
		status = 'UPDATING'
	WHERE
		alert_group_id = i_alert_group_id AND template_id = i_template_id;

	PERFORM code.update_cluster_change(
		i_cid,
		i_rev,
		jsonb_build_object(
			'update_alert',
			jsonb_build_object(
				'alert_group', i_alert_group_id,
				'template', i_template_id
			)
		)
	);

END;
$$ LANGUAGE plpgsql;
