CREATE OR REPLACE FUNCTION code.set_cloud_quota(
    i_cloud_ext_id text,
    i_quota        code.quota,
    i_x_request_id text
) RETURNS code.cloud AS $$
DECLARE
    v_cloud dbaas.clouds;
BEGIN
    UPDATE dbaas.clouds
       SET cpu_quota = coalesce((i_quota).cpu, cpu_quota),
           gpu_quota = coalesce((i_quota).gpu, gpu_quota),
           memory_quota = coalesce((i_quota).memory, memory_quota),
           ssd_space_quota = coalesce((i_quota).ssd_space, ssd_space_quota),
           hdd_space_quota = coalesce((i_quota).hdd_space, hdd_space_quota),
           clusters_quota = coalesce((i_quota).clusters, clusters_quota),
           actual_cloud_rev = actual_cloud_rev + 1
     WHERE cloud_ext_id = i_cloud_ext_id
    RETURNING * INTO v_cloud;

    PERFORM code.log_cloud_rev(v_cloud, i_x_request_id);

    RETURN code.format_cloud(v_cloud);
END;
$$ LANGUAGE plpgsql;
