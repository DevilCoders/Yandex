CREATE OR REPLACE FUNCTION code.add_cloud(
    i_cloud_ext_id text,
    i_quota        code.quota,
    i_x_request_id text
) RETURNS code.cloud AS $$
DECLARE
    v_cloud dbaas.clouds;
BEGIN
    INSERT INTO dbaas.clouds (
        cloud_ext_id,
        cpu_quota,
        gpu_quota,
        memory_quota,
        ssd_space_quota,
        hdd_space_quota,
        clusters_quota,
        actual_cloud_rev
    ) VALUES (
        i_cloud_ext_id,
        (i_quota).cpu,
        (i_quota).gpu,
        (i_quota).memory,
        (i_quota).ssd_space,
        (i_quota).hdd_space,
        (i_quota).clusters,
        1
    ) RETURNING * INTO v_cloud;

    PERFORM code.log_cloud_rev(v_cloud, i_x_request_id);

    RETURN code.format_cloud(v_cloud);
END;
$$ LANGUAGE plpgsql;
