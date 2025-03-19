SELECT
    cloud_id,
    cloud_ext_id,
    cpu_quota,
    gpu_quota,
    memory_quota,
    ssd_space_quota,
    hdd_space_quota,
    clusters_quota,
    memory_used,
    ssd_space_used,
    hdd_space_used,
    cpu_used,
    gpu_used,
    clusters_used,
    code.get_cloud_feature_flags(cloud_id) AS feature_flags
FROM
    code.get_cloud(
        i_cloud_id => %(cloud_id)s,
        i_cloud_ext_id => %(cloud_ext_id)s
    )
