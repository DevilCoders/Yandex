SELECT
       alert_group_id,
       monitoring_folder_id
FROM dbaas.alert_group
WHERE cid = %(cid)s AND dbaas.visible_alert_group_status(status)
