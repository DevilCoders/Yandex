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
    id = %(flavor_id)s
