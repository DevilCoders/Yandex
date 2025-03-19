ALTER TABLE dbaas.worker_queue ADD COLUMN notes text;

CREATE TABLE dbaas.worker_queue_restart_history (
    task_id              text NOT NULL,
    cid                  text NOT NULL,
    create_ts            timestamp with time zone,
    start_ts             timestamp with time zone,
    end_ts               timestamp with time zone,
    worker_id            text,
    task_type            text NOT NULL,
    task_args            jsonb NOT NULL,
    result               boolean,
    changes              jsonb,
    comment              text,
    created_by           text,
    folder_id            bigint NOT NULL,
    operation_type       text NOT NULL,
    metadata             jsonb NOT NULL,
    hidden               boolean NOT NULL,
    version              integer NOT NULL,
    delayed_until        timestamp with time zone,
    required_task_id     text,
    errors               jsonb,
    context              jsonb,
    timeout              interval NOT NULL,
    create_rev           bigint NOT NULL,
    acquire_rev          bigint,
    finish_rev           bigint,
    unmanaged            boolean NOT NULL,
    tracing              text,
    target_rev           bigint,
    config_id            text,
    restart_count        bigint NOT NULL,
    failed_acquire_count bigint NOT NULL,
    notes                text,

    CONSTRAINT pk_worker_queue_restart_history PRIMARY KEY (task_id, restart_count),
    CONSTRAINT fk_worker_queue_restart_history_cid FOREIGN KEY (cid)
        REFERENCES dbaas.clusters(cid) ON DELETE RESTRICT,
    CONSTRAINT fk_worker_queue_restart_history_folder_id FOREIGN KEY (folder_id)
        REFERENCES dbaas.folders(folder_id) ON DELETE RESTRICT,
    CONSTRAINT fk_worker_queue_restart_history_required_task_id FOREIGN KEY (required_task_id)
        REFERENCES dbaas.worker_queue(task_id) ON DELETE RESTRICT,
    CONSTRAINT fk_worker_queue_restart_history_cid_config_id FOREIGN KEY (cid, config_id)
        REFERENCES dbaas.maintenance_tasks(cid, config_id),
    CONSTRAINT check_worker_queue_restart_history_task_id CHECK (
        char_length(task_id) >= 1 AND char_length(task_id) <= 256
    )
);

GRANT SELECT,INSERT,DELETE,UPDATE ON TABLE dbaas.worker_queue_restart_history TO backup_cli;
GRANT SELECT,INSERT,DELETE,UPDATE ON TABLE dbaas.worker_queue_restart_history TO cms;
GRANT SELECT,INSERT,DELETE,UPDATE ON TABLE dbaas.worker_queue_restart_history TO dbaas_api;
GRANT SELECT ON TABLE dbaas.worker_queue_restart_history TO dbaas_support;
GRANT SELECT,INSERT,DELETE,UPDATE ON TABLE dbaas.worker_queue_restart_history TO dbaas_worker;
GRANT SELECT,INSERT,DELETE,UPDATE ON TABLE dbaas.worker_queue_restart_history TO idm_service;
GRANT SELECT ON TABLE dbaas.worker_queue_restart_history TO katan_imp;
GRANT SELECT ON TABLE dbaas.worker_queue_restart_history TO logs_api;
GRANT SELECT ON TABLE dbaas.worker_queue_restart_history TO mdb_health;
GRANT SELECT,INSERT,DELETE,UPDATE ON TABLE dbaas.worker_queue_restart_history TO mdb_maintenance;
GRANT SELECT,INSERT,DELETE,UPDATE ON TABLE dbaas.worker_queue_restart_history TO mdb_ui;
GRANT SELECT ON TABLE dbaas.worker_queue_restart_history TO pillar_config;
GRANT SELECT ON TABLE dbaas.worker_queue_restart_history TO pillar_secrets;

DROP FUNCTION code.restart_task(i_task_id code.task_id, i_force boolean);

CREATE OR REPLACE FUNCTION code.restart_task(
    i_task_id code.task_id,
    i_force   boolean       DEFAULT false,
    i_notes   text          DEFAULT NULL
) RETURNS void AS $$
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

    INSERT INTO dbaas.worker_queue_restart_history
    SELECT
        task_id,
        cid,
        create_ts,
        start_ts,
        end_ts,
        worker_id,
        task_type,
        task_args,
        result,
        changes,
        comment,
        created_by,
        folder_id,
        operation_type,
        metadata,
        hidden,
        version,
        delayed_until,
        required_task_id,
        errors,
        context,
        timeout,
        create_rev,
        acquire_rev,
        finish_rev,
        unmanaged,
        tracing,
        target_rev,
        config_id,
        restart_count,
        failed_acquire_count,
        notes
    FROM dbaas.worker_queue WHERE task_id = i_task_id;

    UPDATE dbaas.worker_queue
       SET changes       = NULL,
           comment       = NULL,
           start_ts      = NULL,
           end_ts        = NULL,
           result        = NULL,
           worker_id     = NULL,
           errors        = NULL,
           restart_count = coalesce(restart_count, 0) + 1,
           notes         = coalesce(i_notes, notes)
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
$$ LANGUAGE plpgsql;

GRANT ALL ON FUNCTION code.restart_task(i_task_id code.task_id, i_force boolean, i_notes text) TO backup_cli;
GRANT ALL ON FUNCTION code.restart_task(i_task_id code.task_id, i_force boolean, i_notes text) TO cms;
GRANT ALL ON FUNCTION code.restart_task(i_task_id code.task_id, i_force boolean, i_notes text) TO dbaas_api;
GRANT ALL ON FUNCTION code.restart_task(i_task_id code.task_id, i_force boolean, i_notes text) TO dbaas_support;
GRANT ALL ON FUNCTION code.restart_task(i_task_id code.task_id, i_force boolean, i_notes text) TO dbaas_worker;
GRANT ALL ON FUNCTION code.restart_task(i_task_id code.task_id, i_force boolean, i_notes text) TO idm_service;
GRANT ALL ON FUNCTION code.restart_task(i_task_id code.task_id, i_force boolean, i_notes text) TO katan_imp;
GRANT ALL ON FUNCTION code.restart_task(i_task_id code.task_id, i_force boolean, i_notes text) TO logs_api;
GRANT ALL ON FUNCTION code.restart_task(i_task_id code.task_id, i_force boolean, i_notes text) TO mdb_downtimer;
GRANT ALL ON FUNCTION code.restart_task(i_task_id code.task_id, i_force boolean, i_notes text) TO mdb_health;
GRANT ALL ON FUNCTION code.restart_task(i_task_id code.task_id, i_force boolean, i_notes text) TO mdb_maintenance;
GRANT ALL ON FUNCTION code.restart_task(i_task_id code.task_id, i_force boolean, i_notes text) TO mdb_ui;
GRANT ALL ON FUNCTION code.restart_task(i_task_id code.task_id, i_force boolean, i_notes text) TO pillar_config;
GRANT ALL ON FUNCTION code.restart_task(i_task_id code.task_id, i_force boolean, i_notes text) TO pillar_secrets;
