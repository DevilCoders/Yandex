CREATE OR REPLACE FUNCTION code.cancel_task(
    i_task_id   code.task_id
) RETURNS void AS $$
BEGIN
    UPDATE dbaas.worker_queue
       SET cancelled = true
     WHERE task_id = i_task_id
       AND result IS NULL
       AND worker_id IS NOT NULL
       AND unmanaged = false;

    IF NOT found THEN
        RAISE EXCEPTION 'Unable to find task % for cancel', i_task_id USING TABLE = 'dbaas.worker_queue';
    END IF;
END;
$$ LANGUAGE plpgsql;
