CREATE OR REPLACE FUNCTION code.get_reject_rev(
    i_task_id   code.task_id,
    i_force     boolean       DEFAULT false
) RETURNS bigint AS $$
DECLARE
    v_task    dbaas.worker_queue;
    v_rev     bigint;
BEGIN
    SELECT *
      INTO v_task
      FROM dbaas.worker_queue
     WHERE task_id = i_task_id
       AND unmanaged = false
       FOR NO KEY UPDATE;

    IF NOT found THEN
        RAISE EXCEPTION 'Unable to get task, probably % is not a task_id', i_task_id
              USING TABLE = 'dbaas.worker_queue';
    END IF;

    IF v_task.result IS NOT NULL THEN
        RAISE EXCEPTION 'This task % is in terminal state, it is %', i_task_id, code.task_status(v_task);
    END IF;

    IF NOT i_force AND coalesce(v_task.restart_count, 0) > 0 THEN
        RAISE EXCEPTION 'This task % has non-zero restart count: %s', i_task_id, v_task.restart_count;
    END IF;

    SELECT finish_rev q
      INTO v_rev
      FROM dbaas.worker_queue q
      JOIN dbaas.clusters c ON (c.cid = q.cid)
     WHERE q.cid = v_task.cid
       AND q.result = true
       AND q.unmanaged = false
       AND q.finish_rev IS NOT NULL
       AND code.visible(c)
  ORDER BY q.finish_rev DESC
     LIMIT 1;

    IF NOT found THEN
        RAISE EXCEPTION 'Unable to find safe revision for rejecting task %s', i_task_id
              USING TABLE = 'dbaas.worker_queue';
    END IF;

    RETURN v_rev;
END;
$$ LANGUAGE plpgsql;
