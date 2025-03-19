SELECT cr.cid,
       cr.name,
       c.type,
       c.env,
       c.created_at,
       cr.network_id,
       cr.status,
       coalesce(pr.value, '{}'::jsonb) "value",
       cr.description,
       cast(code.get_cluster_labels_at_rev(cr.cid, cr.rev) AS jsonb) AS labels,
       mwsr.day AS mw_day,
       mwsr.hour AS mw_hour,
       NULL AS mw_config_id,
       NULL AS mw_max_delay,
       NULL AS mw_delayed_until,
       NULL AS mw_create_ts,
       NULL AS mw_info,
       cr.rev,
       coalesce(br.schedule, '{}'::jsonb) backup_schedule,
       sg_ids user_sgroup_ids,
       coalesce(cr.host_group_ids, '{}') host_group_ids
FROM dbaas.clusters_revs cr
LEFT JOIN dbaas.maintenance_window_settings_revs mwsr USING (cid, rev)
JOIN dbaas.pillar_revs pr USING (cid, rev)
LEFT JOIN dbaas.backup_schedule_revs br USING (cid, rev)
JOIN dbaas.clusters c USING (cid)
, LATERAL (
    SELECT array_agg(sg_ext_id ORDER BY sg_ext_id) sg_ids
    FROM dbaas.sgroups_revs
    WHERE sgroups_revs.cid = c.cid
    AND sg_type = 'user') sg
WHERE cr.cid = %(cid)s AND cr.rev = %(rev)s
