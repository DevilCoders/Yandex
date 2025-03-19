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
    clusters_used
FROM code.set_cloud_quota(
    i_cloud_ext_id => %(cloud_ext_id)s,
    i_quota      => code.make_quota(
        i_cpu       => %(cpu)s::real,
        i_gpu       => %(gpu)s::bigint,
        i_memory    => %(memory)s::bigint,
        i_ssd_space => %(ssd_space)s::bigint,
        i_hdd_space => %(hdd_space)s::bigint,
        i_clusters  => %(clusters)s::bigint
    ),
    i_x_request_id => %(x_request_id)s
)
