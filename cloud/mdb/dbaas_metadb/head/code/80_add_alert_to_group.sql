CREATE OR REPLACE FUNCTION code.add_alert_to_group (
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
	INSERT INTO dbaas.alert (
		alert_group_id,
		template_id,
		notification_channels,
		disabled,
		critical_threshold,
		warning_threshold,
		default_thresholds,
		status
	)
	VALUES (
		i_alert_group_id,
		i_template_id,
		i_notification_channels,
		i_disabled,
		i_crit_threshold,
		i_warn_threshold,
		i_default_thresholds,
		'CREATING'
	);

	PERFORM code.update_cluster_change(
		i_cid,
		i_rev,
		jsonb_build_object(
			'add_alert_to_group',
			jsonb_build_object(
				'alert_group', i_alert_group_id,
				'template_id', i_template_id
			)
		)
	);

END;
$$ LANGUAGE plpgsql;
