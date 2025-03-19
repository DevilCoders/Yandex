CREATE OR REPLACE FUNCTION code.fix_cloud_usage(
    i_cloud_ext_id text,
    i_x_request_id text DEFAULT NULL
) RETURNS code.cloud AS $$
DECLARE
    v_cloud dbaas.clouds;
BEGIN
    -- just lock
    SELECT *
      INTO v_cloud
      FROM dbaas.clouds
     WHERE cloud_ext_id = i_cloud_ext_id
       FOR UPDATE;

    UPDATE dbaas.clouds
       SET cpu_used = cpu,
           gpu_used = gpu,
           memory_used = memory,
           ssd_space_used = ssd_space,
           hdd_space_used = hdd_space,
           clusters_used = clusters,
           actual_cloud_rev = actual_cloud_rev + 1
      FROM code.get_cloud_real_usage(i_cloud_ext_id)
     WHERE cloud_ext_id = i_cloud_ext_id
    RETURNING * INTO v_cloud;

    PERFORM code.log_cloud_rev(v_cloud, i_x_request_id);

    RETURN code.format_cloud(v_cloud);
END;
$$ LANGUAGE plpgsql;
