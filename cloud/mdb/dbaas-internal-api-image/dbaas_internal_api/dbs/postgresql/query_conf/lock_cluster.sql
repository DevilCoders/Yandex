SELECT
    cid,
    name,
    type,
    env,
    created_at,
    network_id,
    status,
    pillar_value "value",
    description,
    labels::jsonb AS labels,
    mw_day,
    mw_hour,
    mw_config_id,
    mw_max_delay,
    mw_delayed_until,
    mw_create_ts,
    mw_info,
    rev,
    backup_schedule,
    user_sgroup_ids,
    host_group_ids,
    deletion_protection
FROM code.lock_cluster(
    i_cid          => %(cid)s,
    i_x_request_id => %(x_request_id)s
)
