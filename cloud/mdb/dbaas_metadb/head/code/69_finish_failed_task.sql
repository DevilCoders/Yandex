CREATE OR REPLACE FUNCTION code.finish_failed_task(
    i_task_id   code.task_id,
    i_worker_id code.worker_id DEFAULT current_user,
    i_comment   text DEFAULT ''
) RETURNS void AS $$
BEGIN
    PERFORM code.restart_task(i_task_id);
    PERFORM code.acquire_task(i_worker_id, i_task_id);
    PERFORM code.finish_task(i_worker_id, i_task_id, true, '{}'::jsonb, i_comment);
END;
$$ LANGUAGE plpgsql
