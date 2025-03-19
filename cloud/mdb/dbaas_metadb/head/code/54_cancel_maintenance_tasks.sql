CREATE OR REPLACE FUNCTION code.cancel_maintenance_tasks(
    i_cid   text,
    i_rev   bigint,
    i_msg   text default 'The job terminated due to cluster or maintenance changes'
) RETURNS void AS $$
BEGIN
    WITH maintenance_task_ids AS (
        UPDATE dbaas.maintenance_tasks
        SET status = 'CANCELED'::dbaas.maintenance_task_status
        WHERE cid = i_cid
          AND status = 'PLANNED'::dbaas.maintenance_task_status
        RETURNING task_id
    )
    UPDATE dbaas.worker_queue
    SET start_ts = now(),
        end_ts = now(),
        result = false,
        changes = '{}'::jsonb,
        errors = concat('[{"code": 1, "type": "Cancelled", "message": "', i_msg, '","exposable": true}]')::jsonb,
        context = NULL,
        finish_rev = i_rev
    WHERE task_id IN (SELECT task_id FROM maintenance_task_ids)
        AND worker_id IS NULL;
END
$$ LANGUAGE plpgsql;
