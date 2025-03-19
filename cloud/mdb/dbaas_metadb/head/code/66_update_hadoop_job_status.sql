CREATE OR REPLACE FUNCTION code.update_hadoop_job_status(
    i_job_id           text,
    i_status           dbaas.hadoop_jobs_status,
    i_application_info jsonb DEFAULT NULL
) RETURNS void AS $$
BEGIN
    UPDATE dbaas.hadoop_jobs
       SET status = i_status,
           application_info = COALESCE(i_application_info, application_info),
           start_ts = CASE WHEN i_status >= 'RUNNING' THEN COALESCE(start_ts, now()) ELSE start_ts END,
           end_ts = CASE WHEN i_status IN ('ERROR', 'DONE', 'CANCELLED') THEN COALESCE(end_ts, now()) ELSE end_ts END
     WHERE job_id = i_job_id
       AND status NOT IN ('ERROR', 'DONE', 'CANCELLED');

    IF NOT found THEN
        RAISE EXCEPTION 'Unable to update job %. Wrong job_id or it is in terminal state', i_job_id;
    END IF;
END;
$$ LANGUAGE plpgsql;
