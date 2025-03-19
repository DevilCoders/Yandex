CREATE OR REPLACE FUNCTION code.reschedule_maintenance_task(
    i_cid                   text,
    i_config_id             text,
    i_plan_ts               timestamp with time zone
) RETURNS void AS $$
BEGIN
    WITH maintenance_task AS (
        UPDATE dbaas.maintenance_tasks
        SET plan_ts=i_plan_ts
        WHERE cid=i_cid AND config_id=i_config_id AND status='PLANNED'::dbaas.maintenance_task_status
        RETURNING task_id
    )
    UPDATE dbaas.worker_queue wq
    SET delayed_until=i_plan_ts
    FROM maintenance_task mt
    WHERE wq.task_id=mt.task_id;

    IF NOT FOUND THEN
        RAISE EXCEPTION 'Planned maintenance task not found % on cluster %', i_config_id, i_cid
            USING TABLE = 'dbaas.maintenance_tasks';
    END IF;
END;
$$ LANGUAGE plpgsql;
