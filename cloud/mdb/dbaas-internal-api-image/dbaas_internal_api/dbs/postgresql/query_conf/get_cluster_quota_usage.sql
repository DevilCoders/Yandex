SELECT
    cpu,
    memory,
    ssd_space,
    hdd_space,
    clusters
FROM
    code.get_cluster_quota_usage(%(cid)s)
