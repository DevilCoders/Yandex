CREATE OR REPLACE FUNCTION code.update_cloud_quota(
    i_cloud_ext_id text,
    i_delta        code.quota,
    i_x_request_id text
) RETURNS code.cloud AS $$
DECLARE
    v_cloud dbaas.clouds;
BEGIN
    UPDATE dbaas.clouds
       SET cpu_quota = code.round_cpu_quota(cpu_quota + coalesce((i_delta).cpu, 0)),
           gpu_quota = gpu_quota + coalesce((i_delta).gpu, 0),
           memory_quota = memory_quota + coalesce((i_delta).memory, 0),
           ssd_space_quota = ssd_space_quota + coalesce((i_delta).ssd_space, 0),
           hdd_space_quota = hdd_space_quota + coalesce((i_delta).hdd_space, 0),
           clusters_quota = clusters_quota + coalesce((i_delta).clusters, 0),
           actual_cloud_rev = actual_cloud_rev + 1
     WHERE cloud_ext_id = i_cloud_ext_id
    RETURNING * INTO v_cloud;

    PERFORM code.log_cloud_rev(v_cloud, i_x_request_id);

    RETURN code.format_cloud(v_cloud);
END;
$$ LANGUAGE plpgsql;
