CREATE OR REPLACE FUNCTION code.acquire_task(
    i_worker_id code.worker_id,
    i_task_id   code.task_id
) RETURNS code.worker_task AS $$
DECLARE
    v_task            dbaas.worker_queue;
    v_folder          dbaas.folders;
    v_new_status      dbaas.cluster_status;
    v_required_result boolean;
    v_rev             bigint;
BEGIN
    v_rev := code.lock_cluster_by_task(i_task_id);

    UPDATE dbaas.worker_queue
       SET start_ts = now(),
           worker_id = i_worker_id,
           acquire_rev = v_rev
     WHERE task_id = i_task_id
       AND start_ts IS NULL
       AND worker_id is NULL
       AND unmanaged = false
    RETURNING * INTO v_task;

    IF NOT found THEN
        RAISE EXCEPTION 'Unable to acquire task %, %', i_task_id, code.describe_not_acquired_task(i_worker_id, i_task_id)
              USING TABLE = 'dbaas.worker_queue';
    END IF;

    IF v_task.required_task_id IS NOT NULL THEN
        SELECT result
          INTO v_required_result
          FROM dbaas.worker_queue
         WHERE task_id = v_task.required_task_id;

        IF (v_required_result IS NULL OR v_required_result = false) THEN
            RAISE EXCEPTION 'Unable to acquire task %, because required task % has result %s', v_task.task_id, v_task.required_task_id, v_required_result;
        END IF;
    END IF;

    SELECT to_status
      INTO v_new_status
      FROM code.cluster_status_acquire_transitions() t
      JOIN dbaas.clusters c ON (c.status = t.from_status)
     WHERE c.cid = v_task.cid
       AND t.action = code.task_type_action(v_task.task_type);

    IF found THEN
        UPDATE dbaas.clusters
           SET status = v_new_status
         WHERE cid = v_task.cid;
    END IF;

    PERFORM code.update_cluster_change(
        (v_task).cid,
        v_rev,
        jsonb_build_object(
            'acquire_task',
            jsonb_build_object(
                'task_id', i_task_id
            )
        )
    );

    PERFORM code.complete_cluster_change(
        (v_task).cid,
        v_rev
    );

    SELECT *
      INTO v_folder
      FROM dbaas.folders
     WHERE folders.folder_id = (v_task).folder_id;

    RETURN code.as_worker_task(
        v_task,
        v_folder);
END;
$$ LANGUAGE plpgsql;
