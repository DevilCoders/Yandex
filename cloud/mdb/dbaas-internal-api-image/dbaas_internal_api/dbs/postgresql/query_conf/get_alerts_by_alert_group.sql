SELECT
       template_id,
       critical_threshold,
       warning_threshold,
       notification_channels,
       disabled
FROM dbaas.alert
WHERE alert_group_id = %(ag_id)s AND dbaas.visible_alert_status(status)
