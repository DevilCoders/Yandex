CREATE OR REPLACE FUNCTION code.verify_clouds_quota_usage() RETURNS TABLE (
    o_cloud_ext_id text,
    o_cloud_id bigint,
    o_cpu real,
    o_gpu bigint,
    o_memory bigint,
    o_ssd_space bigint,
    o_hdd_space bigint,
    o_clusters bigint
) AS $$
SELECT cloud_ext_id,
       cloud_id,
       cpu - cpu_used,
       gpu - gpu_used,
       memory - memory_used,
       hdd_space - hdd_space_used,
       ssd_space - ssd_space_used,
       clusters - clusters_used
  FROM dbaas.clouds,
       code.get_cloud_real_usage(cloud_ext_id) real_usage
 WHERE code.get_usage_from_cloud(clouds) != real_usage;
$$ LANGUAGE SQL STABLE;
