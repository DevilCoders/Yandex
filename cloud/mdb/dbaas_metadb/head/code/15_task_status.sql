CREATE OR REPLACE FUNCTION code.task_status(
    q dbaas.worker_queue
) RETURNS code.operation_status AS $$
SELECT
    (CASE
    WHEN q.end_ts IS NULL AND q.start_ts IS NULL
        THEN 'PENDING'
    WHEN q.start_ts IS NOT NULL AND q.end_ts IS NULL
        THEN 'RUNNING'
    WHEN q.start_ts IS NOT NULL AND q.end_ts IS NOT NULL AND q.result IS FALSE
        THEN 'FAILED'
    WHEN q.start_ts IS NOT NULL AND q.end_ts IS NOT NULL AND q.result IS TRUE
        THEN 'DONE'
    END)::code.operation_status;
$$ LANGUAGE SQL IMMUTABLE;
