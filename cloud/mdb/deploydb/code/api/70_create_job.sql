CREATE OR REPLACE FUNCTION code.create_job(
    i_ext_job_id    text,
    i_command_id    bigint
) RETURNS code.job AS $$
DECLARE
    v_command deploy.commands;
    v_job deploy.jobs;
    v_fqdn text;
    v_job_result deploy.job_results;
    v_ts timestamptz;
BEGIN
    v_command := code._acquire_command(i_command_id);
    v_fqdn = (SELECT fqdn FROM deploy.minions
        WHERE minion_id = v_command.minion_id);

    IF (v_command).status != 'AVAILABLE' THEN
        RAISE EXCEPTION 'Command status is not AVAILABLE, id %', i_command_id
              USING ERRCODE = code._error_invalid_state(), TABLE = 'deploy.commands';
    END IF;

    v_ts := clock_timestamp();

    INSERT INTO deploy.jobs
        (ext_job_id, command_id, status, created_at, updated_at, last_running_check_at)
    VALUES
        (i_ext_job_id, i_command_id, 'RUNNING', v_ts, v_ts, v_ts)
    RETURNING * INTO v_job;

    UPDATE deploy.commands
        SET status = 'RUNNING',
            updated_at = v_ts,
            last_dispatch_attempt_at = v_ts
    WHERE command_id = i_command_id
    RETURNING * INTO v_command;

    -- to workaround race between create_job and create_job_result
    INSERT INTO deploy.job_results
        (ext_job_id, fqdn, order_id, recorded_at)
    VALUES
        (i_ext_job_id, v_fqdn, 1, v_ts)
    ON CONFLICT (ext_job_id, fqdn, order_id) DO NOTHING
    RETURNING * INTO v_job_result;

    IF NOT found THEN
        -- in this case result already set for fqnd + ext_job_id
        SELECT * INTO v_job_result
        FROM deploy.job_results
        WHERE ext_job_id = i_ext_job_id AND fqdn = v_fqdn AND order_id = 1;

        v_job := code._update_job_status(v_job, v_job_result, v_ts);
    END IF;

    RETURN code._as_job(v_job);
END;
$$ LANGUAGE plpgsql;
