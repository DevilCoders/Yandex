CREATE OR REPLACE FUNCTION code.format_cloud(
    c dbaas.clouds
) RETURNS code.cloud AS $$
SELECT
    $1.cloud_id,
    $1.cloud_ext_id,
    0::bigint, -- cloud_rev
    $1.cpu_quota,
    $1.gpu_quota,
    $1.memory_quota,
    $1.ssd_space_quota,
    $1.hdd_space_quota,
    $1.clusters_quota,
    $1.cpu_used,
    $1.gpu_used,
    $1.memory_used,
    $1.ssd_space_used,
    $1.hdd_space_used,
    $1.clusters_used;
$$ LANGUAGE SQL IMMUTABLE;
