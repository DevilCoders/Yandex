SELECT
    d.disk_type_ext_id as disk_type_id,
    CASE WHEN min(lower(r.disk_size_range)) IS NULL THEN NULL ELSE int8range(min(lower(r.disk_size_range)), max(upper(r.disk_size_range))) END AS disk_size_range,
    array_remove(array_agg(DISTINCT ds ORDER BY ds), NULL) AS disk_sizes,
    f.name AS preset_id,
    f.cpu_limit,
    f.memory_limit
FROM 
    dbaas.valid_resources r
    JOIN dbaas.flavors f ON (f.id = r.flavor)
    JOIN dbaas.disk_type d USING (disk_type_id)
    JOIN dbaas.geo g USING (geo_id)
    LEFT JOIN LATERAL unnest(r.disk_sizes) as ds ON TRUE
WHERE
    r.cluster_type = %(cluster_type)s
    AND ( %(role)s IS NULL OR r.role = %(role)s )
    AND ( %(flavor_type)s IS NULL OR f.type = %(flavor_type)s )
    AND ( %(generation)s IS NULL OR f.generation = %(generation)s )
    AND ( %(min_cpu)s IS NULL OR f.cpu_limit >= %(min_cpu)s )
    AND ( %(feature_flags)s IS NULL OR r.feature_flag IS NULL OR r.feature_flag = ANY(%(feature_flags)s::text[]) )
    AND ( %(decommissioning_flavors)s IS NULL OR NOT f.name = ANY(%(decommissioning_flavors)s::text[]) )
GROUP BY
    r.cluster_type,
    r.role,
    d.disk_type_ext_id,
    f.id
HAVING
    array_agg(g.name) @> %(zones)s
ORDER BY
    vtype, 
    f.type,
    generation DESC,
    f.cpu_limit,
    f.memory_limit,
    f.name
LIMIT 1
