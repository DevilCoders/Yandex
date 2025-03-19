CREATE OR REPLACE FUNCTION code.finish_failed_task(
    i_task_id   code.task_id,
    i_worker_id code.worker_id DEFAULT current_user,
    i_comment   text DEFAULT ''
) RETURNS void AS $$
BEGIN
    PERFORM code.restart_task(i_task_id);
    PERFORM code.acquire_task(i_worker_id, i_task_id);
    PERFORM code.finish_task(i_worker_id, i_task_id, true, '{}'::jsonb, i_comment);
END;
$$ LANGUAGE plpgsql;

GRANT ALL ON FUNCTION code.finish_failed_task(i_task_id code.task_id, i_worker_id code.worker_id, i_comment text) TO backup_cli;
GRANT ALL ON FUNCTION code.finish_failed_task(i_task_id code.task_id, i_worker_id code.worker_id, i_comment text) TO cms;
GRANT ALL ON FUNCTION code.finish_failed_task(i_task_id code.task_id, i_worker_id code.worker_id, i_comment text) TO dbaas_api;
GRANT ALL ON FUNCTION code.finish_failed_task(i_task_id code.task_id, i_worker_id code.worker_id, i_comment text) TO dbaas_support;
GRANT ALL ON FUNCTION code.finish_failed_task(i_task_id code.task_id, i_worker_id code.worker_id, i_comment text) TO dbaas_worker;
GRANT ALL ON FUNCTION code.finish_failed_task(i_task_id code.task_id, i_worker_id code.worker_id, i_comment text) TO idm_service;
GRANT ALL ON FUNCTION code.finish_failed_task(i_task_id code.task_id, i_worker_id code.worker_id, i_comment text) TO katan_imp;
GRANT ALL ON FUNCTION code.finish_failed_task(i_task_id code.task_id, i_worker_id code.worker_id, i_comment text) TO logs_api;
GRANT ALL ON FUNCTION code.finish_failed_task(i_task_id code.task_id, i_worker_id code.worker_id, i_comment text) TO mdb_downtimer;
GRANT ALL ON FUNCTION code.finish_failed_task(i_task_id code.task_id, i_worker_id code.worker_id, i_comment text) TO mdb_health;
GRANT ALL ON FUNCTION code.finish_failed_task(i_task_id code.task_id, i_worker_id code.worker_id, i_comment text) TO mdb_maintenance;
GRANT ALL ON FUNCTION code.finish_failed_task(i_task_id code.task_id, i_worker_id code.worker_id, i_comment text) TO mdb_ui;
GRANT ALL ON FUNCTION code.finish_failed_task(i_task_id code.task_id, i_worker_id code.worker_id, i_comment text) TO pillar_config;
GRANT ALL ON FUNCTION code.finish_failed_task(i_task_id code.task_id, i_worker_id code.worker_id, i_comment text) TO pillar_secrets;
