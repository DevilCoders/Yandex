UPDATE
    mdb.containers
SET
    cpu_guarantee = coalesce(%(cpu_guarantee)s, cpu_guarantee),
    cpu_limit = coalesce(%(cpu_limit)s, cpu_limit),
    memory_guarantee = coalesce(%(memory_guarantee)s, memory_guarantee),
    memory_limit = coalesce(%(memory_limit)s, memory_limit),
    hugetlb_limit = coalesce(%(hugetlb_limit)s, hugetlb_limit),
    net_guarantee = coalesce(%(net_guarantee)s, net_guarantee),
    net_limit = coalesce(%(net_limit)s, net_limit),
    io_limit = coalesce(%(io_limit)s, io_limit),
    extra_properties = coalesce(%(extra_properties)s, extra_properties),
    generation = coalesce(%(generation)s, generation),
    bootstrap_cmd = coalesce(%(bootstrap_cmd)s, bootstrap_cmd),
    secrets = coalesce(%(secrets)s, secrets),
    secrets_expire = coalesce(%(secrets_expire)s, secrets_expire),
    project_id = coalesce(%(project_id)s, project_id),
    managing_project_id = coalesce(%(managing_project_id)s, managing_project_id)
WHERE
    fqdn = %(fqdn)s
RETURNING
    dom0
