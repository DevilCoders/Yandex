CREATE OR REPLACE FUNCTION code.timeout_jobs(
    i_limit bigint
) RETURNS SETOF code.job AS $$
DECLARE
    v_jobs          deploy.jobs;
    v_minion_fqdn   text;
    v_ts            timestamptz;
BEGIN
    v_ts := clock_timestamp();

    FOR v_jobs IN
        SELECT jobs.*
          FROM deploy.jobs
          JOIN deploy.commands
        USING (command_id)
          JOIN deploy.shipment_commands
        USING (shipment_command_id)
        WHERE jobs.status = 'RUNNING'
          AND jobs.created_at + shipment_commands.timeout <= v_ts
        LIMIT i_limit
        FOR UPDATE SKIP LOCKED
    LOOP
        SELECT minions.fqdn INTO v_minion_fqdn
          FROM deploy.jobs
          JOIN deploy.commands
        USING (command_id)
          JOIN deploy.shipment_commands
        USING (shipment_command_id)
          JOIN deploy.minions
        USING (minion_id)
          WHERE jobs.job_id = (v_jobs).job_id;

        PERFORM code._create_job_result(
            i_ext_job_id    => (v_jobs).ext_job_id,
            i_fqdn          => v_minion_fqdn,
            i_status        => 'TIMEOUT',
            i_result        => '{}',
            i_ts            => v_ts
        );

        RETURN NEXT code._as_job((SELECT jobs FROM deploy.jobs WHERE job_id = (v_jobs).job_id));
    END LOOP;
END;
$$ LANGUAGE plpgsql;
