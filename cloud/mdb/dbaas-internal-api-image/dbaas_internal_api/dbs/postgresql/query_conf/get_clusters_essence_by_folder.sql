SELECT
    c.cid,
    c.name AS name,
    c.type AS type,
    env,
    c.created_at AS created_at,
    pillar_value AS value,
    network_id,
    c.status AS status,
    description,
    labels::jsonb,
    mw_day,
    mw_hour,
    mw_config_id,
    mw_max_delay,
    mw_delayed_until,
    mw_create_ts,
    mw_info,
    backup_schedule,
    user_sgroup_ids,
    host_group_ids,
    c.deletion_protection,
    (SELECT json_agg(json_build_object(
                'roles', roles::text[],
                'shard_id', s.shard_id
            )
            ORDER BY s.created_at
        ) as shards
        FROM dbaas.shards s
        JOIN dbaas.subclusters sc USING (subcid)
        WHERE sc.cid = c.cid
    ) AS oldest_shards,
    (SELECT json_agg(json_build_object(
                'fqdn', h.fqdn,
                'shard_id', h.shard_id,
                'roles', roles::text[],
                'space_limit', space_limit,
                'flavor_name', f.name,
                'disk_type_id', disk_type_ext_id
            )
        ) AS info
        FROM dbaas.subclusters s
        JOIN dbaas.hosts h     USING (subcid)
        JOIN dbaas.disk_type d USING (disk_type_id)
        JOIN dbaas.flavors f   ON    (h.flavor = f.id)
        WHERE s.cid = c.cid
    ) AS hosts_info
FROM code.get_clusters(
    i_folder_id         => %(folder_id)s,
    i_cid               => %(cid)s,
    i_cluster_name      => %(cluster_name)s,
    i_env               => %(env)s,
    i_cluster_type      => %(cluster_type)s,
    i_page_token_name   => %(page_token_name)s,
    i_limit             => %(limit)s
) c
