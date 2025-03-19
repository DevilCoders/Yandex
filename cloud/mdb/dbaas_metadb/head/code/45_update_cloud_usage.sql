CREATE OR REPLACE FUNCTION code.update_cloud_usage(
    i_cloud_id     bigint,
    i_delta  code.quota,
    i_x_request_id text
) RETURNS code.cloud AS $$
DECLARE
    v_cloud dbaas.clouds;
BEGIN
    UPDATE dbaas.clouds
       SET cpu_used = code.round_cpu_quota(cpu_used + coalesce((i_delta).cpu, 0)),
           gpu_used = gpu_used + coalesce((i_delta).gpu, 0),
           memory_used = memory_used + coalesce((i_delta).memory, 0),
           ssd_space_used = ssd_space_used + coalesce((i_delta).ssd_space, 0),
           hdd_space_used = hdd_space_used + coalesce((i_delta).hdd_space, 0),
           clusters_used = clusters_used + coalesce((i_delta).clusters, 0),
           actual_cloud_rev = actual_cloud_rev + 1
     WHERE cloud_id = i_cloud_id
    RETURNING * INTO v_cloud;

    PERFORM code.log_cloud_rev(v_cloud, i_x_request_id);

    RETURN code.format_cloud(v_cloud);
END;
$$ LANGUAGE plpgsql;
