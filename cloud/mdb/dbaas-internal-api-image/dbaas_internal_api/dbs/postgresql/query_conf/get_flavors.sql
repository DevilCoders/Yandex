SELECT
    id,
    cpu_guarantee,
    cpu_limit,
    (cpu_guarantee * 100 / cpu_limit)::int AS cpu_fraction,
    gpu_limit,
    memory_guarantee,
    memory_limit,
    network_guarantee,
    network_limit,
    io_limit,
    io_cores_limit,
    name,
    name as description,
    vtype,
    type,
    generation,
    platform_id
FROM
    dbaas.flavors
WHERE
    ( %(instance_type_name)s IS NULL OR name = %(instance_type_name)s )
    AND ( (%(last_cpu_limit)s IS NULL AND %(last_id)s IS NULL)
          OR (cpu_limit, id) > (%(last_cpu_limit)s, %(last_id)s) )
ORDER BY cpu_limit, id
LIMIT %(limit)s
