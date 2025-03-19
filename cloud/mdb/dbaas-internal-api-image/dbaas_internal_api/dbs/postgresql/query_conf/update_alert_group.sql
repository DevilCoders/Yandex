UPDATE
	dbaas.alert_group
SET
	monitoring_folder_id = %(monitoring_folder_id)s
WHERE alert_group_id = %(alert_group_id)s
