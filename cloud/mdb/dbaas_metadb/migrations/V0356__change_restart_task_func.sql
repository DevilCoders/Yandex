-- Changed functions
CREATE OR REPLACE FUNCTION code.restart_task(i_task_id code.task_id, i_force boolean DEFAULT false)
 RETURNS void
 LANGUAGE plpgsql
AS $function$
DECLARE
    v_task_row   dbaas.worker_queue;
    v_cluster    dbaas.clusters;
    v_rev        bigint;
BEGIN
    SELECT *
      INTO v_task_row
      FROM dbaas.worker_queue
     WHERE task_id = i_task_id
       AND unmanaged = false
       FOR NO KEY UPDATE;

    IF NOT found THEN
        RAISE EXCEPTION 'Unable to restart task,'
                        ' probably % is not task_id', i_task_id
              USING TABLE = 'dbaas.worker_queue';
    END IF;

    IF v_task_row.result IS NULL THEN
        RAISE EXCEPTION 'This task % is not in terminal state,'
                        ' it %', i_task_id, code.task_status(v_task_row);
    END IF;

    v_rev := (code.lock_cluster(v_task_row.cid)).rev;

    IF NOT i_force THEN
        SELECT *
          INTO v_cluster
          FROM dbaas.clusters
         WHERE cid = v_task_row.cid;

        IF NOT EXISTS (
            SELECT to_status
              FROM code.cluster_status_acquire_transitions() t
             WHERE from_status = v_cluster.status
               AND action = code.task_type_action(v_task_row.task_type))
        THEN
            RAISE EXCEPTION 'Invalid cluster status % for restarting %', v_cluster.status, v_task_row.task_type
                    USING TABLE = 'dbaas.worker_queue';
        END IF;

        /* a non-empty config_id means that the task is a maintenance task */
        IF v_task_row.config_id IS NOT NULL AND NOT EXISTS (
            SELECT 1
            FROM dbaas.maintenance_tasks
            WHERE task_id = i_task_id
              AND status = 'FAILED'::dbaas.maintenance_task_status)
        THEN
            RAISE EXCEPTION 'Restarting a maintenance task is allowed only for active maintenance in "FAILED" status'
                USING TABLE = 'dbaas.worker_queue';
        END IF;

    END IF;

    UPDATE dbaas.worker_queue
       SET changes       = NULL,
           comment       = NULL,
           start_ts      = NULL,
           end_ts        = NULL,
           result        = NULL,
           worker_id     = NULL,
           errors        = NULL,
           restart_count = coalesce(restart_count, 0) + 1
     WHERE task_id = i_task_id;

    PERFORM code.update_cluster_change(
        (v_task_row).cid,
        v_rev,
        jsonb_build_object(
            'restart_task',
            jsonb_build_object(
                'task_id', i_task_id
            )
        )
    );

    PERFORM code.complete_cluster_change(
        (v_task_row).cid,
        v_rev
    );
END;
$function$;
