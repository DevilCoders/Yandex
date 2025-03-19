CREATE OR REPLACE FUNCTION code.update_task(
    i_worker_id code.worker_id,
    i_task_id   code.task_id,
    i_changes   jsonb,
    i_comment   text,
    i_context   jsonb DEFAULT NULL
) RETURNS void AS $$
BEGIN
    UPDATE dbaas.worker_queue
       SET changes = i_changes,
           comment = i_comment,
           context = i_context
     WHERE task_id = i_task_id
       AND unmanaged = false
       AND worker_id IS NOT DISTINCT FROM i_worker_id;

    IF NOT found THEN
        RAISE EXCEPTION 'Unable to update task %, %', i_task_id, code.describe_not_acquired_task(i_worker_id, i_task_id)
              USING TABLE = 'dbaas.worker_queue';
    END IF;
END;
$$ LANGUAGE plpgsql;
