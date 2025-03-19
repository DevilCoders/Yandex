CREATE OR REPLACE FUNCTION code.log_cloud_rev(
    i_cloud        dbaas.clouds,
    i_x_request_id text
) RETURNS void AS $$
INSERT INTO dbaas.clouds_revs
    (cloud_id, cloud_rev,
     cpu_quota, gpu_quota, memory_quota, ssd_space_quota,
     hdd_space_quota, clusters_quota,
     cpu_used, gpu_used, memory_used, ssd_space_used,
     hdd_space_used, clusters_used,
     x_request_id)
SELECT
     $1.cloud_id, $1.actual_cloud_rev,
     $1.cpu_quota, $1.gpu_quota, $1.memory_quota, $1.ssd_space_quota,
     $1.hdd_space_quota, $1.clusters_quota,
     $1.cpu_used, $1.gpu_used, $1.memory_used, $1.ssd_space_used,
     $1.hdd_space_used, $1.clusters_used,
     i_x_request_id;
$$ LANGUAGE SQL;
