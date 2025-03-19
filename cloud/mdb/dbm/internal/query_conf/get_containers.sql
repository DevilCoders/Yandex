SELECT
    fqdn,
    dom0,
    cluster_name,
    cpu_guarantee,
    cpu_limit,
    memory_guarantee,
    memory_limit,
    hugetlb_limit,
    net_guarantee,
    net_limit,
    io_limit,
    extra_properties,
    generation,
    project_id,
    managing_project_id
FROM
    mdb.containers
WHERE
    (%(fqdn)s IS NULL OR fqdn ~ %(fqdn)s)
    AND (%(dom0)s IS NULL OR dom0 ~ %(dom0)s)
    AND (%(cluster_name)s IS NULL OR cluster_name ~ %(cluster_name)s)
    AND (%(cpu_guarantee)s IS NULL OR cpu_guarantee = %(cpu_guarantee)s)
    AND (%(cpu_limit)s IS NULL OR cpu_limit = %(cpu_limit)s)
    AND (%(memory_guarantee)s IS NULL OR memory_guarantee = %(memory_guarantee)s)
    AND (%(memory_limit)s IS NULL OR memory_limit = %(memory_limit)s)
    AND (%(hugetlb_limit)s IS NULL OR hugetlb_limit = %(hugetlb_limit)s)
    AND (%(net_guarantee)s IS NULL OR net_guarantee = %(net_guarantee)s)
    AND (%(net_limit)s IS NULL OR net_limit = %(net_limit)s)
    AND (%(io_limit)s IS NULL OR io_limit = %(io_limit)s)
    AND (%(extra_properties)s IS NULL OR extra_properties @> %(extra_properties)s)
    AND (%(generation)s IS NULL OR generation = %(generation)s)
    AND (%(project_id)s IS NULL OR project_id = %(project_id)s)
    AND (%(managing_project_id)s IS NULL OR managing_project_id = %(managing_project_id)s)
ORDER BY
    fqdn
LIMIT
    %(limit)s
OFFSET
    %(offset)s
