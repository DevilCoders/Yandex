SELECT
    f.name AS id,
    f.cpu_limit AS cores,
    f.io_cores_limit AS io_cores,
    (f.cpu_guarantee * 100 / f.cpu_limit)::int AS core_fraction,
    f.gpu_limit AS gpus,
    f.memory_limit AS memory,
    f.type AS type,
    f.generation AS generation,
    f.platform_id AS platform_id,
    array_agg(DISTINCT role)::text[] AS roles,
    array_agg(DISTINCT g.name) AS zone_ids
FROM
    dbaas.valid_resources vr
    JOIN dbaas.flavors f ON (f.id = vr.flavor)
    JOIN dbaas.geo g USING (geo_id)
WHERE
    cluster_type = %(cluster_type)s
    AND (%(resource_preset_id)s IS NULL OR f.name = %(resource_preset_id)s)
    AND ( (%(page_token_cores)s IS NULL AND %(page_token_id)s IS NULL)
          OR (f.cpu_limit, f.name) > (%(page_token_cores)s, %(page_token_id)s))
    AND f.name != All (%(decommissioning_flavors)s)
    AND ( %(feature_flags)s IS NULL OR vr.feature_flag IS NULL OR vr.feature_flag = ANY(%(feature_flags)s::text[]) )
GROUP BY
    f.name,
    cores,
    io_cores,
    core_fraction,
    gpus,
    memory,
    type,
    generation,
    platform_id
ORDER BY
    type,
    generation DESC,
    cores,
    io_cores,
    core_fraction,
    gpus,
    memory,
    f.name
LIMIT
    coalesce(%(limit)s::int, 100)
