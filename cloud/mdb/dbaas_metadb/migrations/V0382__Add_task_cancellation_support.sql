ALTER TABLE dbaas.worker_queue ADD COLUMN cancelled boolean;
ALTER TABLE dbaas.worker_queue ADD CONSTRAINT check_cancelled CHECK (
    (((result IS NULL) AND (cancelled) OR (NOT cancelled)))
);

ALTER TABLE dbaas.worker_queue_restart_history ADD COLUMN cancelled boolean;

CREATE OR REPLACE FUNCTION code.finish_task(
    i_worker_id code.worker_id,
    i_task_id   code.task_id,
    i_result    boolean,
    i_changes   jsonb,
    i_comment   text,
    i_errors    jsonb DEFAULT NULL
) RETURNS void AS $$
DECLARE
    v_task dbaas.worker_queue;
    v_new_status dbaas.cluster_status;
    v_rev        bigint;
BEGIN
    v_rev := code.lock_cluster_by_task(i_task_id);

    UPDATE dbaas.worker_queue
       SET end_ts = now(),
           result = i_result,
           changes = i_changes,
           comment = i_comment,
           errors = i_errors,
           context = null,
           cancelled = null,
           finish_rev = v_rev
     WHERE task_id = i_task_id
       AND worker_id IS NOT DISTINCT FROM i_worker_id
       AND result IS NULL
       AND unmanaged = false
    RETURNING * INTO v_task;

    IF NOT found THEN
        RAISE EXCEPTION 'Unable to finish task %, %', i_task_id, code.describe_not_acquired_task(i_worker_id, i_task_id)
              USING TABLE = 'dbaas.worker_queue';
    END IF;

    IF v_task.config_id IS NOT NULL THEN
        UPDATE dbaas.maintenance_tasks
        SET status = (CASE WHEN i_result THEN 'COMPLETED'::dbaas.maintenance_task_status ELSE 'FAILED'::dbaas.maintenance_task_status END)
        WHERE cid = v_task.cid AND config_id=v_task.config_id;
    END IF;

    SELECT to_status
      INTO v_new_status
      FROM code.cluster_status_finish_transitions() t
      JOIN dbaas.clusters c ON (c.status = t.from_status)
     WHERE c.cid = v_task.cid
       AND t.action = code.task_type_action(v_task.task_type)
       AND t.result = i_result;

    IF found THEN
        UPDATE dbaas.clusters
           SET status = v_new_status
         WHERE cid = v_task.cid;
    END IF;

    PERFORM code.update_cluster_change(
        (v_task).cid,
        v_rev,
        jsonb_build_object(
            'finish_task',
            jsonb_build_object(
                'task_id', i_task_id
            )
        )
    );

    PERFORM code.complete_cluster_change(
        (v_task).cid,
        v_rev
    );
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION code.cancel_task(
    i_task_id   code.task_id
) RETURNS void AS $$
BEGIN
    UPDATE dbaas.worker_queue
       SET cancelled = true
     WHERE task_id = i_task_id
       AND result IS NULL
       AND worker_id IS NOT NULL
       AND unmanaged = false;

    IF NOT found THEN
        RAISE EXCEPTION 'Unable to find task % for cancel', i_task_id USING TABLE = 'dbaas.worker_queue';
    END IF;
END;
$$ LANGUAGE plpgsql;

GRANT ALL ON FUNCTION code.cancel_task(i_task_id code.task_id) TO backup_cli;
GRANT ALL ON FUNCTION code.cancel_task(i_task_id code.task_id) TO cms;
GRANT ALL ON FUNCTION code.cancel_task(i_task_id code.task_id) TO dbaas_api;
GRANT ALL ON FUNCTION code.cancel_task(i_task_id code.task_id) TO dbaas_support;
GRANT ALL ON FUNCTION code.cancel_task(i_task_id code.task_id) TO dbaas_worker;
GRANT ALL ON FUNCTION code.cancel_task(i_task_id code.task_id) TO idm_service;
GRANT ALL ON FUNCTION code.cancel_task(i_task_id code.task_id) TO katan_imp;
GRANT ALL ON FUNCTION code.cancel_task(i_task_id code.task_id) TO logs_api;
GRANT ALL ON FUNCTION code.cancel_task(i_task_id code.task_id) TO mdb_downtimer;
GRANT ALL ON FUNCTION code.cancel_task(i_task_id code.task_id) TO mdb_health;
GRANT ALL ON FUNCTION code.cancel_task(i_task_id code.task_id) TO mdb_maintenance;
GRANT ALL ON FUNCTION code.cancel_task(i_task_id code.task_id) TO mdb_ui;
GRANT ALL ON FUNCTION code.cancel_task(i_task_id code.task_id) TO pillar_config;
GRANT ALL ON FUNCTION code.cancel_task(i_task_id code.task_id) TO pillar_secrets;
