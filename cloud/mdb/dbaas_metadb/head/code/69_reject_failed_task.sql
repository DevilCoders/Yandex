CREATE OR REPLACE FUNCTION code.reject_failed_task(
    i_task_id   code.task_id,
    i_worker_id code.worker_id DEFAULT current_user,
    i_comment   text DEFAULT ''
) RETURNS void AS $$
BEGIN
    PERFORM code.restart_task(i_task_id);
    PERFORM code.acquire_task(i_worker_id, i_task_id);
    PERFORM code.reject_task(i_worker_id, i_task_id, '{}'::jsonb, i_comment, i_force => true);
END;
$$ LANGUAGE plpgsql;
