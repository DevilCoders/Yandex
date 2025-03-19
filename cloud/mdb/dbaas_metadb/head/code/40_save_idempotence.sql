CREATE OR REPLACE FUNCTION code.save_idempotence(
    i_operation_id      text,
    i_folder_id         bigint,
    i_user_id           text,
    i_idempotence_data  code.idempotence_data DEFAULT NULL
) RETURNS code.operation AS $$
DECLARE
    v_task    dbaas.worker_queue;
    v_cluster dbaas.clusters;
BEGIN
    IF i_idempotence_data.idempotence_id IS NOT NULL AND i_idempotence_data.request_hash IS NOT NULL
    THEN
        INSERT INTO dbaas.idempotence (
            idempotence_id,
            task_id,
            folder_id,
            user_id,
            request_hash
        )
        VALUES (
            i_idempotence_data.idempotence_id,
            i_operation_id,
            i_folder_id,
            i_user_id,
            i_idempotence_data.request_hash
        );
    END IF;

    RETURN code.as_operation(v_task, v_cluster);
END;
$$ LANGUAGE plpgsql;
