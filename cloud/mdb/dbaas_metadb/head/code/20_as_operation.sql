CREATE OR REPLACE FUNCTION code.as_operation(
    q dbaas.worker_queue,
    c dbaas.clusters
) RETURNS code.operation AS $$
SELECT
    q.task_id,
    coalesce(q.cid, (q.task_args->>'cid')::text),
    q.cid,
    c.type,
    c.env,
    q.operation_type,
    q.created_by,
    q.create_ts,
    q.start_ts,
    coalesce(q.end_ts, q.start_ts, q.create_ts),
    code.task_status(q) AS status,
    q.metadata,
    q.task_args,
    q.hidden,
    q.required_task_id,
    q.errors;
$$ LANGUAGE SQL IMMUTABLE;
