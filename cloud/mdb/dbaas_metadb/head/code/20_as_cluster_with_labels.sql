CREATE OR REPLACE FUNCTION code.as_cluster_with_labels(
    c   dbaas.clusters,
    b   jsonb,
    pl  jsonb,
    l   code.label[],
    mws dbaas.maintenance_window_settings,
    mt  dbaas.maintenance_tasks,
    sgs text[]
) RETURNS code.cluster_with_labels AS $$
SELECT
    c.cid,
    c.actual_rev,
    c.next_rev,
    c.name,
    c.type,
    c.folder_id,
    c.env,
    c.created_at,
    c.status,
    coalesce(pl, '{}'),
    c.network_id,
    c.description,
    coalesce(l, '{}'::code.label[]),
    mws.day AS mw_day,
    mws.hour AS mw_hour,
    mt.config_id AS mw_config_id,
    mt.max_delay AS mw_max_delay,
    mt.plan_ts AS mw_delayed_until,
    mt.create_ts AS mw_create_ts,
    mt.info AS mw_info,
    json_build_object(
        'day', mws.day,
        'hour', mws.hour
    )::jsonb,
    coalesce(b, '{}'),
    coalesce(sgs, '{}'),
    coalesce(c.host_group_ids, '{}'),
    c.deletion_protection;
$$ LANGUAGE SQL IMMUTABLE;

CREATE OR REPLACE FUNCTION code.as_cluster_with_labels(
    dbaas.clusters
) RETURNS code.cluster_with_labels AS $$
SELECT code.as_cluster_with_labels(
    $1,
    (SELECT backup_schedule.schedule FROM dbaas.backup_schedule WHERE cid = ($1).cid),
    (SELECT pillar.value FROM dbaas.pillar WHERE cid = ($1).cid),
    (SELECT array_agg(
              (label_key, label_value)::code.label
              ORDER BY label_key, label_value) la
      FROM dbaas.cluster_labels cl
     WHERE cl.cid = ($1).cid),
    mws,
    mt,
    ARRAY(SELECT sg_ext_id
            FROM dbaas.sgroups
           WHERE sgroups.cid=($1).cid AND sg_type='user'
           ORDER BY sg_ext_id)
)
FROM (VALUES(($1).cid)) AS c (cid)
LEFT JOIN dbaas.maintenance_window_settings mws USING (cid)
LEFT JOIN dbaas.maintenance_tasks mt ON ($1).cid = mt.cid AND mt.status='PLANNED'::dbaas.maintenance_task_status;
$$ LANGUAGE SQL STABLE;
