CREATE OR REPLACE FUNCTION code.finish_unmanaged_task(
    i_task_id   code.task_id,
    i_result    boolean,
    i_changes   jsonb,
    i_comment   text,
    i_errors    jsonb DEFAULT NULL
) RETURNS void AS $$
BEGIN
    UPDATE dbaas.worker_queue
       SET end_ts = now(),
           result = i_result,
           changes = i_changes,
           comment = i_comment,
           errors = i_errors,
           context = null
     WHERE task_id = i_task_id
       AND unmanaged = true;

    IF NOT found THEN
        RAISE EXCEPTION 'Unable to finish unmanaged task %', i_task_id
              USING TABLE = 'dbaas.worker_queue';
    END IF;

END;
$$ LANGUAGE plpgsql;
