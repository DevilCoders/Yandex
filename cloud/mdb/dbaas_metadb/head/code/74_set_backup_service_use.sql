CREATE OR REPLACE FUNCTION code.set_backup_service_use(
    i_cid       text,
    i_val       bool,
    i_reason    text DEFAULT NULL
) RETURNS void AS $$
DECLARE
    v_cluster record;
BEGIN
    v_cluster := code.lock_cluster(
        i_cid => i_cid,
        i_x_request_id => i_reason
    );
    UPDATE dbaas.backup_schedule
       SET schedule = jsonb_set(
        v_cluster.backup_schedule,
        '{use_backup_service}',
        CAST(i_val as TEXT)::jsonb
    )
    WHERE cid = i_cid;
    PERFORM code.update_cluster_change(
        i_cid,
        v_cluster.rev,
        jsonb_build_object(
            'update_backup_schedule',
            jsonb_build_object(
                'cid', i_cid
            )
        )
    );
    PERFORM code.complete_cluster_change(i_cid, v_cluster.rev);
    PERFORM code.add_finished_operation_for_current_rev(
        i_operation_id => gen_random_uuid()::text,
        i_cid => i_cid,
        i_folder_id => v_cluster.folder_id,
        i_operation_type => (v_cluster.type::text || '_modify'),
        i_metadata => '{}'::jsonb,
        i_user_id => 'backup_cli',
        i_version => 2,
        i_hidden => true,
        i_rev => v_cluster.rev
    );
    RETURN;
END;
$$ LANGUAGE plpgsql;
