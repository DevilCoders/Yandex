CREATE OR REPLACE FUNCTION code.running_job_missing(
    i_ext_job_id    text,
    i_fqdn          text,
    i_fail_on_count int
) RETURNS code.job AS $$
DECLARE
    v_job           deploy.jobs;
    v_updated_job   deploy.jobs;
BEGIN
    SELECT * INTO v_job FROM code._get_job(i_ext_job_id, i_fqdn);

    IF v_job IS NULL THEN
        RAISE EXCEPTION 'Unable to find job ext id %, fqdn %', i_ext_job_id, i_fqdn
            USING ERRCODE = code._error_not_found(), TABLE = 'deploy.jobs';
    END IF;

    -- Are we too late?
    IF (v_job).status != 'RUNNING' THEN
        RETURN code._as_job(v_job);
    END IF;

    -- We sync on commands so lock it
    PERFORM code._acquire_command((v_job).command_id);

    -- Increment fail counter
    -- We use v_updated_job so we don't overwrite v_job with failed update
    -- Job could have finished between RUNNING check and command acquire, so recheck that we actually updated it
    UPDATE deploy.jobs
        SET running_checks_failed = running_checks_failed + 1
    WHERE job_id = (v_job).job_id AND status = 'RUNNING'
        RETURNING * INTO v_updated_job;

    -- Did we actually update anything?
    IF NOT found THEN
        RETURN code._as_job(v_job);
    END IF;

    -- Copy so that we won't fail in variable names
    v_job = v_updated_job;

    -- Do nothing if we didn't reach fail counter
    IF (v_job).running_checks_failed < i_fail_on_count THEN
        RETURN code._as_job(v_job);
    END IF;

    -- Fail job
    PERFORM code._create_job_result(
        i_ext_job_id    => i_ext_job_id,
        i_fqdn          => i_fqdn,
        i_status        => 'NOTRUNNING',
        i_result        => '{}',
        i_ts            => clock_timestamp());

    RETURN code._as_job((SELECT jobs FROM deploy.jobs WHERE job_id = (v_job).job_id));
END;
$$ LANGUAGE plpgsql;
