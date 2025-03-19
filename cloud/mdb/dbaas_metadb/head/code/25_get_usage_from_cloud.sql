CREATE OR REPLACE FUNCTION code.get_usage_from_cloud(
    dbaas.clouds
) RETURNS code.quota AS $$
SELECT code.make_quota(
    i_cpu       => $1.cpu_used,
    i_gpu       => $1.gpu_used,
    i_memory    => $1.memory_used,
    i_ssd_space => $1.ssd_space_used,
    i_hdd_space => $1.hdd_space_used,
    i_clusters  => $1.clusters_used
);
$$ LANGUAGE SQL IMMUTABLE;
