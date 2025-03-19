CREATE OR REPLACE FUNCTION code.release_task(
    i_worker_id code.worker_id,
    i_task_id   code.task_id,
    i_context   jsonb
) RETURNS void AS $$
BEGIN
    UPDATE dbaas.worker_queue
       SET changes     = NULL,
           comment     = NULL,
           start_ts    = NULL,
           end_ts      = NULL,
           result      = NULL,
           worker_id   = NULL,
           errors      = NULL,
           context     = i_context
     WHERE task_id = i_task_id
       AND unmanaged = false
       AND worker_id IS NOT DISTINCT FROM i_worker_id;

    IF NOT found THEN
        RAISE EXCEPTION 'Unable to release task %, %', i_task_id, code.describe_not_acquired_task(i_worker_id, i_task_id)
              USING TABLE = 'dbaas.worker_queue';
    END IF;
END;
$$ LANGUAGE plpgsql;
