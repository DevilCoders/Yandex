CREATE OR REPLACE FUNCTION code.increment_failed_acquire_count(
    i_task_id   code.task_id
) RETURNS void AS $$
BEGIN
    UPDATE dbaas.worker_queue
       SET failed_acquire_count = failed_acquire_count + 1
     WHERE task_id = i_task_id
       AND unmanaged = false;

    IF NOT found THEN
        RAISE EXCEPTION 'Unable to increment failed acquire count for task %: task not found', i_task_id
              USING TABLE = 'dbaas.worker_queue';
    END IF;
END;
$$ LANGUAGE plpgsql;
