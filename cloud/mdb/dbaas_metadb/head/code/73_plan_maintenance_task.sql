CREATE OR REPLACE FUNCTION code.plan_maintenance_task(
    i_task_id               text,
    i_cid                   text,
    i_config_id             text,
    i_folder_id             bigint,
    i_operation_type        text,
    i_task_type             text,
    i_task_args             jsonb,
    i_version               integer,
    i_metadata              jsonb,
    i_user_id               text,
    i_rev                   bigint,
    i_target_rev            bigint,
    i_plan_ts               timestamp with time zone,
    i_info                  text,
    i_create_ts             timestamp with time zone DEFAULT now(),
    i_timeout               interval DEFAULT '1 hour',
    i_max_delay             timestamp with time zone DEFAULT now() + INTERVAL '21 0:00:00'
) RETURNS void AS $$
BEGIN
    INSERT INTO dbaas.maintenance_tasks
        (max_delay, cid, config_id, task_id, plan_ts, create_ts, info)
    VALUES
        (i_max_delay, i_cid, i_config_id, i_task_id, i_plan_ts, i_create_ts, i_info)
    ON CONFLICT (cid, config_id)
        DO UPDATE
        SET task_id = i_task_id,
            max_delay = i_max_delay,
            plan_ts = i_plan_ts,
            create_ts = i_create_ts,
            info = i_info,
            status = 'PLANNED'::dbaas.maintenance_task_status;

    INSERT INTO dbaas.worker_queue (
        task_id,
        cid,
        folder_id,
        task_type,
        task_args,
        created_by,
        operation_type,
        metadata,
        hidden,
        version,
        delayed_until,
        timeout,
        create_rev,
        unmanaged,
        target_rev,
        config_id
    )
    VALUES (
       i_task_id,
       i_cid,
       i_folder_id,
       i_task_type,
       i_task_args,
       i_user_id,
       i_operation_type,
       i_metadata,
       false,
       i_version,
       i_plan_ts,
       i_timeout,
       i_rev,
       false,
       i_target_rev,
       i_config_id
    );
END;
$$ LANGUAGE plpgsql;
