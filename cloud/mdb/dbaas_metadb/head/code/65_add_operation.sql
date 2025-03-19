CREATE OR REPLACE FUNCTION code.add_operation(
    i_operation_id          text,
    i_cid                   text,
    i_folder_id             bigint,
    i_operation_type        text,
    i_task_type             text,
    i_task_args             jsonb,
    i_metadata              jsonb,
    i_user_id               text,
    i_version               integer,
    i_hidden                boolean                  DEFAULT false,
    i_time_limit            interval                 DEFAULT NULL,
    i_idempotence_data      code.idempotence_data    DEFAULT NULL,
    i_delay_by              interval                 DEFAULT NULL,
    i_required_operation_id text                     DEFAULT NULL,
    i_rev                   bigint                   DEFAULT NULL,
    i_tracing               text                     DEFAULT NULL
) RETURNS code.operation AS $$
DECLARE
    v_task       dbaas.worker_queue;
    v_cluster    dbaas.clusters;
    v_new_status dbaas.cluster_status;
BEGIN
    SELECT *
      INTO v_cluster
      FROM dbaas.clusters
     WHERE cid = i_cid;

    -- change status only for realtime task (i_delay_by IS NULL)
    IF i_delay_by IS NULL THEN
        SELECT to_status
          INTO v_new_status
          FROM code.cluster_status_add_transitions() t
         WHERE from_status = (v_cluster).status
           AND action = code.task_type_action(i_task_type);

        IF NOT found THEN
            RAISE EXCEPTION 'Invalid cluster status % for adding %', (v_cluster).status, i_task_type
                  USING TABLE = 'dbaas.worker_queue';
        END IF;

        -- update v_cluster, cause we update our cluster
        UPDATE dbaas.clusters
           SET status = v_new_status
         WHERE cid = i_cid
        RETURNING * INTO v_cluster;
    END IF;

    INSERT INTO dbaas.worker_queue (
        task_id,
        cid,
        folder_id,
        task_type,
        task_args,
        created_by,
        operation_type,
        metadata,
        hidden,
        version,
        delayed_until,
        required_task_id,
        timeout,
        create_rev,
        unmanaged,
        tracing
    )
    VALUES (
        i_operation_id,
        i_cid,
        i_folder_id,
        i_task_type,
        i_task_args,
        i_user_id,
        i_operation_type,
        i_metadata,
        i_hidden,
        i_version,
        now() + i_delay_by,
        i_required_operation_id,
        coalesce(i_time_limit, '1 hour'),
        i_rev,
        false,
        i_tracing
    )
    RETURNING * INTO v_task;

    PERFORM code.save_idempotence(
        i_operation_id     => i_operation_id,
        i_folder_id        => i_folder_id,
        i_user_id          => i_user_id,
        i_idempotence_data => i_idempotence_data
    );

    PERFORM code.update_cluster_change(
        i_cid,
        i_rev,
        jsonb_build_object(
            'add_operation',
            jsonb_build_object(
                'operation_id', i_operation_id
            )
        )
    );
    RETURN code.as_operation(v_task, v_cluster);
END;
$$ LANGUAGE plpgsql;
