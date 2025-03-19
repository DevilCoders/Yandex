CREATE OR REPLACE FUNCTION code._update_job_status(
    job deploy.jobs,
    job_result deploy.job_results,
    ts timestamptz
) RETURNS deploy.jobs AS $$
DECLARE
    v_command deploy.commands;
    v_job deploy.jobs;
    v_status text;
BEGIN
    v_command := code._acquire_command((job).command_id);

    -- (job_result).status may be NULL if some one create empty job_result
    -- record and call this function
    IF (job).status != 'RUNNING' OR (job_result).status IS NULL THEN
        RETURN job;
    END IF;

    v_status := CASE
        WHEN (job_result).status = 'SUCCESS' THEN 'DONE'
        WHEN (job_result).status IN ('FAILURE', 'NOTRUNNING') THEN 'ERROR'
        WHEN (job_result).status = 'TIMEOUT' THEN 'TIMEOUT'
    END;

    UPDATE deploy.jobs
       SET status = v_status::deploy.job_status,
           updated_at = ts
     WHERE job_id = (job).job_id
    RETURNING * INTO v_job;

    PERFORM code._update_command_status(v_command, (v_job).status, ts);

    RETURN v_job;
END;
$$ LANGUAGE plpgsql;
