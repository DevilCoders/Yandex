CREATE OR REPLACE FUNCTION code.reject_task(
    i_worker_id code.worker_id,
    i_task_id   code.task_id,
    i_changes   jsonb,
    i_comment   text,
    i_errors    jsonb           DEFAULT NULL,
    i_force     boolean         DEFAULT false
) RETURNS void AS $$
DECLARE
    v_rev       bigint;
    v_new_rev   bigint;
    v_cluster   dbaas.clusters;
    v_cloud     dbaas.clouds;
    v_new_cloud dbaas.clouds;
    v_task      dbaas.worker_queue;
BEGIN
    -- Lock cloud without getting revision
    SELECT c.*
      INTO v_cloud
      FROM dbaas.worker_queue q
           JOIN dbaas.folders f ON (f.folder_id = q.folder_id)
           JOIN dbaas.clouds c ON (c.cloud_id = f.cloud_id)
     WHERE q.task_id = i_task_id
       FOR NO KEY UPDATE;

    IF NOT found THEN
        RAISE EXCEPTION 'Unable to lock cloud for task %', i_task_id
              USING TABLE = 'dbaas.worker_queue';
    END IF;

    -- Also lock (possibly) new cloud
    SELECT cl.*
      INTO v_new_cloud
      FROM dbaas.worker_queue q
           JOIN dbaas.clusters c ON (q.cid = c.cid)
           JOIN dbaas.folders f ON (f.folder_id = c.folder_id)
           JOIN dbaas.clouds cl ON (cl.cloud_id = f.cloud_id)
     WHERE q.task_id = i_task_id
       FOR NO KEY UPDATE;

    -- Lock cluster without getting revision
    SELECT c.*
      INTO v_cluster
      FROM dbaas.worker_queue q
           JOIN dbaas.clusters c ON (q.cid = c.cid)
     WHERE q.task_id = i_task_id
       FOR NO KEY UPDATE;

    IF NOT found THEN
        RAISE EXCEPTION 'Unable to lock cluster for task %', i_task_id
              USING TABLE = 'dbaas.worker_queue';
    END IF;

    v_rev := code.get_reject_rev(i_task_id, i_force);

    v_new_rev := code.revert_cluster_to_rev(v_cluster.cid, v_rev, i_task_id || ' reject');

    PERFORM code.fix_cloud_usage(v_cloud.cloud_ext_id);

    IF (v_cloud).cloud_ext_id != (v_new_cloud).cloud_ext_id THEN
        PERFORM code.fix_cloud_usage(v_new_cloud.cloud_ext_id);
    END IF;

    UPDATE dbaas.worker_queue
       SET end_ts = now(),
           result = false,
           changes = i_changes,
           comment = i_comment,
           errors = i_errors,
           context = null,
           finish_rev = v_new_rev
     WHERE task_id = i_task_id
       AND worker_id IS NOT DISTINCT FROM i_worker_id
       AND unmanaged = false
     RETURNING * INTO v_task;

    IF NOT found THEN
        RAISE EXCEPTION 'Unable to reject task %, %', i_task_id, code.describe_not_acquired_task(i_worker_id, i_task_id)
              USING TABLE = 'dbaas.worker_queue';
    END IF;

    IF v_task.config_id IS NOT NULL THEN
        UPDATE dbaas.maintenance_tasks
        SET status = 'REJECTED'
        WHERE cid = v_task.cid AND config_id=v_task.config_id;
    END IF;

    UPDATE dbaas.worker_queue
       SET start_ts = now(),
           end_ts = now(),
           worker_id = i_worker_id,
           result = false,
           comment = 'Failed due to reject of ' || i_task_id,
           errors = i_errors
     WHERE required_task_id = i_task_id
       AND unmanaged = false;
END;
$$ LANGUAGE plpgsql;
