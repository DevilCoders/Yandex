SELECT
    c.cid,
    c.name,
    c.type,
    c.env,
    c.created_at,
    c.network_id,
    c.status,
    coalesce(p.value, '{}'::jsonb) "value",
    description,
    code.get_cluster_labels(c.cid)::jsonb AS labels,
    mws.day AS mw_day,
    mws.hour AS mw_hour,
    mt.config_id AS mw_config_id,
    mt.max_delay AS mw_max_delay,
    mt.plan_ts AS mw_delayed_until,
    mt.create_ts AS mw_create_ts,
    mt.info AS mw_info,
    code.rev(c),
    coalesce(b.schedule, '{}'::jsonb) backup_schedule,
    sg_ids user_sgroup_ids,
    c.host_group_ids,
    c.deletion_protection,
    c.monitoring_cloud_id
FROM
    dbaas.clusters c
    LEFT JOIN dbaas.pillar p ON (c.cid = p.cid)
    LEFT JOIN dbaas.maintenance_window_settings mws ON (c.cid = mws.cid)
    LEFT JOIN dbaas.maintenance_tasks mt ON (c.cid = mt.cid AND mt.status='PLANNED'::dbaas.maintenance_task_status)
    LEFT JOIN dbaas.backup_schedule b ON (c.cid = b.cid)
    , LATERAL (
        SELECT array_agg(sg_ext_id ORDER BY sg_ext_id) sg_ids
        FROM dbaas.sgroups
        WHERE sgroups.cid = c.cid
        AND sg_type = 'user') sg
WHERE
    (%(cid)s IS NULL OR c.cid = %(cid)s)
    AND (%(cluster_name)s IS NULL OR c.name = %(cluster_name)s)
    AND (%(cluster_type)s IS NULL OR type = %(cluster_type)s)
    AND code.match_visibility(c, %(visibility)s)
