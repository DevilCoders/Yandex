WITH valid_resources AS (
    SELECT
        flavor,
        disk_type_id,
        geo_id,
        CASE WHEN min(lower(r.disk_size_range)) IS NULL THEN NULL ELSE int8range(min(lower(r.disk_size_range)), max(upper(r.disk_size_range))) END AS disk_size_range,
        array_remove(array_agg(DISTINCT ds ORDER BY ds), NULL) AS disk_sizes,
        min_hosts,
        max_hosts,
        array_agg(role ORDER BY role) as roles
    FROM dbaas.valid_resources r
    LEFT JOIN LATERAL unnest(r.disk_sizes) as ds ON TRUE
    WHERE
          cluster_type = %(cluster_type)s
          AND ( %(feature_flags)s IS NULL OR r.feature_flag IS NULL OR r.feature_flag = ANY(%(feature_flags)s::text[]) )
    GROUP BY
        flavor,
        disk_type_id,
        geo_id,
        min_hosts,
        max_hosts
)
SELECT
    f.name as id,
    vr.roles::text[] as roles,
    f.cpu_limit as cores,
    f.gpu_limit as gpus,
    f.memory_limit as memory,
    d.disk_type_ext_id as disk_type_id,
    array_agg(g.name) as zone_ids,
    vr.disk_size_range as disk_size_range,
    vr.disk_sizes as disk_sizes,
    vr.min_hosts as min_hosts,
    vr.max_hosts as max_hosts
FROM
    valid_resources vr
    JOIN dbaas.flavors f ON (f.id = vr.flavor)
    JOIN dbaas.geo g USING (geo_id)
    JOIN dbaas.disk_type d USING (disk_type_id)
WHERE
    (%(role)s IS NULL OR %(role)s = ANY(vr.roles))
    AND (%(resource_preset_id)s IS NULL OR f.name = %(resource_preset_id)s)
    AND (%(geo)s IS NULL OR g.name = %(geo)s)
    AND (%(disk_type_ext_id)s IS NULL
         OR d.disk_type_ext_id = %(disk_type_ext_id)s)
GROUP BY
    f.name,
    roles,
    cores,
    gpus,
    memory,
    disk_type_ext_id,
    disk_size_range,
    disk_sizes,
    min_hosts,
    max_hosts
ORDER BY
    cores,
    gpus,
    memory,
    f.name
