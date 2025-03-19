INSERT INTO mdb.containers (
    dom0, fqdn, cluster_name, cpu_guarantee,
    cpu_limit, memory_guarantee, memory_limit,
    hugetlb_limit, net_guarantee, net_limit,
    io_limit, extra_properties, bootstrap_cmd,
    secrets, secrets_expire, generation,
    project_id, managing_project_id)
VALUES (
    %(dom0)s, %(fqdn)s, %(cluster_name)s, %(cpu_guarantee)s,
    %(cpu_limit)s, %(memory_guarantee)s, %(memory_limit)s,
    %(hugetlb_limit)s, %(net_guarantee)s, %(net_limit)s,
    %(io_limit)s, %(extra_properties)s, %(bootstrap_cmd)s,
    %(secrets)s, now() + '1 hour'::interval, %(generation)s,
    %(project_id)s, %(managing_project_id)s)
