SELECT alert.alert_group_id,
       f.folder_ext_id,
       alert.warning_threshold,
       alert.critical_threshold,
       alert.status,
       alert.notification_channels,
       alert.alert_ext_id,
       alert.template_id,
       defa.name,
       ag.monitoring_folder_id,
       ag.cid,
       defa.template_version,
       defa.description
FROM dbaas.alert alert
         INNER JOIN dbaas.alert_group ag on ag.alert_group_id = alert.alert_group_id
         INNER JOIN dbaas.clusters cl on ag.cid = cl.cid
         INNER JOIN dbaas.folders f on f.folder_id = cl.folder_id
         JOIN dbaas.default_alert defa on alert.template_id = defa.template_id
WHERE ag.cid = %(cid)s;
