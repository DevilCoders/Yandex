CREATE OR REPLACE FUNCTION code.add_finished_operation_for_current_rev(
    i_operation_id      text,
    i_cid               text,
    i_folder_id         bigint,
    i_operation_type    text,
    i_metadata          jsonb,
    i_user_id           text,
    i_version           integer,
    i_hidden            boolean DEFAULT false,
    i_idempotence_data  code.idempotence_data DEFAULT NULL,
    i_rev               bigint DEFAULT NULL
) RETURNS code.operation AS $$
DECLARE
    v_task    dbaas.worker_queue;
    v_cluster dbaas.clusters;
BEGIN
    INSERT INTO dbaas.worker_queue (
        task_id,
        cid,
        folder_id,
        result,
        start_ts,
        end_ts,
        task_type,
        task_args,
        created_by,
        operation_type,
        metadata,
        hidden,
        version,
        timeout,
        create_rev,
        acquire_rev,
        finish_rev,
        unmanaged
    )
    VALUES (
        i_operation_id,
        i_cid,
        i_folder_id,
        true,
        now(),
        now(),
        code.finished_operation_task_type(),
        '{}',
        i_user_id,
        i_operation_type,
        i_metadata,
        i_hidden,
        i_version,
        '1 second',
        i_rev,
        i_rev,
        i_rev,
        false
    )
    RETURNING * INTO v_task;

    SELECT *
      INTO v_cluster
      FROM dbaas.clusters
     WHERE cid = i_cid;

    PERFORM code.save_idempotence(
        i_operation_id     => i_operation_id,
        i_folder_id        => i_folder_id,
        i_user_id          => i_user_id,
        i_idempotence_data => i_idempotence_data
    );

    RETURN code.as_operation(v_task, v_cluster);
END;
$$ LANGUAGE plpgsql;
