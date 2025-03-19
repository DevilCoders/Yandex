CREATE OR REPLACE FUNCTION code.get_running_jobs_and_mark_them(
    i_master    text,
    i_running   interval,
    i_limit     bigint
) RETURNS TABLE(ext_job_id text, minion text) AS $$
DECLARE
    v_job_id bigint;
    v_ts timestamptz;
BEGIN
    v_ts := clock_timestamp();

    FOR v_job_id, ext_job_id, minion IN
        SELECT jobs.job_id, jobs.ext_job_id, minions.fqdn
        FROM deploy.jobs
        JOIN deploy.commands
            USING (command_id)
        JOIN deploy.minions
            USING (minion_id)
        JOIN deploy.masters
            USING (master_id)
        WHERE masters.fqdn = i_master
          AND jobs.status = 'RUNNING'
          AND jobs.last_running_check_at + i_running <= v_ts
        ORDER BY jobs.last_running_check_at ASC
        LIMIT i_limit
        FOR UPDATE OF jobs SKIP LOCKED
    LOOP
        UPDATE deploy.jobs
        SET last_running_check_at = v_ts,
            updated_at = v_ts
        WHERE job_id = v_job_id;

        RETURN NEXT;
    END LOOP;
END;
$$ LANGUAGE plpgsql;
