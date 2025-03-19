CREATE OR REPLACE FUNCTION code.get_cluster_quota_usage(
    i_cid text
) RETURNS code.quota AS $$
-- we count usage by guarantee
SELECT code.make_quota(
    i_cpu       => coalesce(cpu::real, 0),
    i_gpu       => coalesce(gpu::bigint, 0),
    i_memory    => coalesce(memory::bigint, 0),
    i_network   => coalesce(network::bigint, 0),
    i_io        => coalesce(io::bigint, 0),
    i_ssd_space => coalesce(ssd_space::bigint, 0),
    i_hdd_space => coalesce(hdd_space::bigint, 0),
    i_clusters  => coalesce(clusters_count, 0))
  FROM (
      SELECT sum(cpu_guarantee) AS cpu,
             sum(gpu_limit) AS gpu,
             sum(memory_guarantee) AS memory,
             sum(network_guarantee) AS network,
             sum(io_limit) AS io,
             sum(space_limit) FILTER (WHERE quota_type = 'ssd'::dbaas.space_quota_type) AS ssd_space,
             sum(space_limit) FILTER (WHERE quota_type = 'hdd'::dbaas.space_quota_type) AS hdd_space,
             count(DISTINCT cid) AS clusters_count
        FROM dbaas.clusters
        JOIN dbaas.subclusters USING (cid)
        JOIN dbaas.hosts USING (subcid)
        JOIN dbaas.disk_type USING (disk_type_id)
        JOIN dbaas.flavors ON (hosts.flavor = flavors.id)
       WHERE clusters.cid = i_cid
         AND clusters.type != ANY(code.unquoted_clusters())) a;
$$ LANGUAGE SQL STABLE;
