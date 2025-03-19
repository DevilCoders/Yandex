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
FROM code.update_cloud_usage(
    i_cloud_id   => %(cloud_id)s,
    i_delta      => code.make_quota(
        i_cpu       => coalesce(%(add_cpu)s, 0.0)::real,
        i_gpu       => coalesce(%(add_gpu)s, 0)::bigint,
        i_memory    => coalesce(%(add_memory)s, 0)::bigint,
        i_ssd_space => coalesce(%(add_ssd_space)s, 0)::bigint,
        i_hdd_space => coalesce(%(add_hdd_space)s, 0)::bigint,
        i_clusters  => coalesce(%(add_clusters)s, 0)::bigint
    ),
    i_x_request_id => %(x_request_id)s
)
