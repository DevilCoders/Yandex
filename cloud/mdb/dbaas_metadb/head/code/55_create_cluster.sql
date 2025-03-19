CREATE OR REPLACE FUNCTION code.create_cluster(
    i_cid                 text,
    i_name                text,
    i_type                dbaas.cluster_type,
    i_env                 dbaas.env_type,
    i_public_key          bytea,
    i_network_id          text,
    i_folder_id           bigint,
    i_description         text,
    i_x_request_id        text DEFAULT NULL,
    i_host_group_ids      text[] DEFAULT NULL,
    i_deletion_protection boolean DEFAULT FALSE,
    i_monitoring_cloud_id TEXT DEFAULT NULL
) RETURNS code.cluster_with_labels AS $$
DECLARE
    v_cluster dbaas.clusters;
    -- cast, cause I got a problems with fresh plpgsql_check
    -- it complaints: `Hidden casting can be a performance issue.`
    v_rev     constant bigint := CAST(1 AS bigint);
BEGIN
    INSERT INTO dbaas.clusters (
        cid, name, type,
        env, public_key, network_id,
        folder_id, description,
        actual_rev, next_rev, host_group_ids,
        deletion_protection, monitoring_cloud_id
    )
    VALUES (
        i_cid, i_name, i_type,
        i_env, i_public_key, i_network_id,
        i_folder_id, i_description,
        v_rev, v_rev, i_host_group_ids,
        i_deletion_protection, i_monitoring_cloud_id
    )
    RETURNING * INTO v_cluster;

    INSERT INTO dbaas.clusters_changes (
        cid,
        rev,
        changes,
        x_request_id
    ) VALUES (
        i_cid,
        v_rev,
        jsonb_build_array(
            jsonb_build_object(
                'create_cluster', jsonb_build_object()
            )
        ),
        i_x_request_id
    );

    RETURN code.as_cluster_with_labels(
        v_cluster, null, null, null, null, null, '{}'
    );
END;
$$ LANGUAGE plpgsql;
