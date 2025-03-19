SELECT
       alert_group_id,
       monitoring_folder_id
FROM
       dbaas.alert_group
WHERE
       cid = %(cid)s AND managed
