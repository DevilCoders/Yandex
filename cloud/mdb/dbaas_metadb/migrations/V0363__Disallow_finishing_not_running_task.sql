CREATE OR REPLACE FUNCTION code.finish_task(
    i_worker_id code.worker_id,
    i_task_id   code.task_id,
    i_result    boolean,
    i_changes   jsonb,
    i_comment   text,
    i_errors    jsonb DEFAULT NULL
) RETURNS void AS $$
DECLARE
    v_task dbaas.worker_queue;
    v_new_status dbaas.cluster_status;
    v_rev        bigint;
BEGIN
    v_rev := code.lock_cluster_by_task(i_task_id);

    UPDATE dbaas.worker_queue
       SET end_ts = now(),
           result = i_result,
           changes = i_changes,
           comment = i_comment,
           errors = i_errors,
           context = null,
           finish_rev = v_rev
     WHERE task_id = i_task_id
       AND worker_id IS NOT DISTINCT FROM i_worker_id
       AND result IS NULL
       AND unmanaged = false
    RETURNING * INTO v_task;

    IF NOT found THEN
        RAISE EXCEPTION 'Unable to finish task %, %', i_task_id, code.describe_not_acquired_task(i_worker_id, i_task_id)
              USING TABLE = 'dbaas.worker_queue';
    END IF;

    IF v_task.config_id IS NOT NULL THEN
        UPDATE dbaas.maintenance_tasks
        SET status = (CASE WHEN i_result THEN 'COMPLETED'::dbaas.maintenance_task_status ELSE 'FAILED'::dbaas.maintenance_task_status END)
        WHERE cid = v_task.cid AND config_id=v_task.config_id;
    END IF;

    SELECT to_status
      INTO v_new_status
      FROM code.cluster_status_finish_transitions() t
      JOIN dbaas.clusters c ON (c.status = t.from_status)
     WHERE c.cid = v_task.cid
       AND t.action = code.task_type_action(v_task.task_type)
       AND t.result = i_result;

    IF found THEN
        UPDATE dbaas.clusters
           SET status = v_new_status
         WHERE cid = v_task.cid;
    END IF;

    PERFORM code.update_cluster_change(
        (v_task).cid,
        v_rev,
        jsonb_build_object(
            'finish_task',
            jsonb_build_object(
                'task_id', i_task_id
            )
        )
    );

    PERFORM code.complete_cluster_change(
        (v_task).cid,
        v_rev
    );

END;
$$ LANGUAGE plpgsql;
