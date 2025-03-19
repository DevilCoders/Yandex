SELECT
    r.role,
    d.disk_type_ext_id as disk_type_id,
    g.name AS geo_name,
    CASE WHEN min(lower(r.disk_size_range)) IS NULL THEN NULL ELSE int8range(min(lower(r.disk_size_range)), max(upper(r.disk_size_range))) END AS disk_size_range,
    array_remove(array_agg(DISTINCT ds ORDER BY ds), NULL) AS disk_sizes,
    f.name AS preset_id,
    f.cpu_limit,
    (f.cpu_guarantee * 100 / f.cpu_limit)::int AS cpu_fraction,
    f.io_cores_limit,
    f.gpu_limit,
    f.memory_limit,
    r.min_hosts,
    r.max_hosts,
    f.type,
    f.generation
FROM  dbaas.valid_resources r
    JOIN dbaas.flavors f ON (f.id = r.flavor)
    JOIN dbaas.geo g USING (geo_id)
    JOIN dbaas.disk_type d USING (disk_type_id)
    LEFT JOIN LATERAL unnest(r.disk_sizes) as ds ON TRUE
WHERE
    r.cluster_type = %(cluster_type)s
    AND ( %(feature_flags)s IS NULL OR r.feature_flag IS NULL OR r.feature_flag = ANY(%(feature_flags)s::text[]) )
GROUP BY
    r.cluster_type,
    r.role,
    d.disk_type_ext_id,
    f.id,
    g.name,
    f.cpu_limit,
    f.cpu_guarantee,
    f.io_cores_limit,
    f.gpu_limit,
    f.memory_limit,
    r.min_hosts,
    r.max_hosts,
    f.type,
    f.generation

ORDER BY
    f.type,
    generation DESC,
    f.cpu_limit,
    cpu_fraction,
    f.memory_limit,
    f.name
