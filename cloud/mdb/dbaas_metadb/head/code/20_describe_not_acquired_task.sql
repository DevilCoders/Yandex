CREATE OR REPLACE FUNCTION code.describe_not_acquired_task(
    i_worker_id code.worker_id,
    i_task_id   code.task_id
) RETURNS text AS $$
DECLARE
    v_task dbaas.worker_queue;
BEGIN
    SELECT *
      INTO v_task
      FROM dbaas.worker_queue
     WHERE task_id = i_task_id;

    IF found AND v_task.unmanaged THEN
        RETURN 'this task is unmanaged';
    END IF;

    IF found THEN
        RETURN format(
            'another worker %L != %L acquire it',
            v_task.worker_id,
            i_worker_id
        );
    END IF;
    RETURN 'cause no task with this id found';
END;
$$ LANGUAGE plpgsql STABLE;
