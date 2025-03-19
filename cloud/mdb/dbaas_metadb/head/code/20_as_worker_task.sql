CREATE OR REPLACE FUNCTION code.as_worker_task(
    q dbaas.worker_queue, f dbaas.folders
) RETURNS code.worker_task AS $$
SELECT
    q.task_id,
    q.cid,
    q.task_type,
    q.task_args,
    q.created_by,
    q.context,
    f.folder_ext_id,
    q.hidden,
    q.timeout,
    q.tracing,
    q.restart_count,
    q.create_ts
$$ LANGUAGE SQL IMMUTABLE;
