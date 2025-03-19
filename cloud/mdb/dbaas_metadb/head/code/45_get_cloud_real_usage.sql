CREATE OR REPLACE FUNCTION code.get_cloud_real_usage(
    i_cloud_ext_id text
) RETURNS code.quota AS $$
-- we count usage by guarantee
SELECT code.make_quota(
    i_cpu       => coalesce(cpu::real, 0),
    i_gpu       => coalesce(gpu::bigint, 0),
    i_memory    => coalesce(memory::bigint, 0),
    i_ssd_space => coalesce(ssd_space::bigint, 0),
    i_hdd_space => coalesce(hdd_space::bigint, 0),
    i_clusters  => coalesce(clusters_count, 0))
  FROM (
      SELECT sum(cpu_guarantee) AS cpu,
             sum(gpu_limit) AS gpu,
             sum(memory_guarantee) AS memory,
             sum(space_limit) FILTER (WHERE quota_type = 'ssd'::dbaas.space_quota_type) AS ssd_space,
             sum(space_limit) FILTER (WHERE quota_type = 'hdd'::dbaas.space_quota_type) AS hdd_space,
             count(DISTINCT cid) AS clusters_count
        FROM dbaas.clouds
        JOIN dbaas.folders USING (cloud_id)
        JOIN dbaas.clusters USING (folder_id)
        JOIN dbaas.subclusters USING (cid)
        LEFT JOIN dbaas.hosts USING (subcid)
        LEFT JOIN dbaas.disk_type USING (disk_type_id)
        LEFT JOIN dbaas.flavors ON (hosts.flavor = flavors.id)
       WHERE dbaas.visible_cluster_status(clusters.status)
         AND clusters.type != ANY(code.unquoted_clusters())
         AND cloud_ext_id = i_cloud_ext_id) a;
$$ LANGUAGE SQL STABLE;
