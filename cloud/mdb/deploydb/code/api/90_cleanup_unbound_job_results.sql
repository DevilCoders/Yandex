CREATE OR REPLACE FUNCTION code.cleanup_unbound_job_results(
    i_age interval,
    i_limit bigint
) RETURNS bigint AS $$
DECLARE
    v_ts            timestamptz;
    v_deleted         bigint;
BEGIN
    v_ts := clock_timestamp();

    DELETE FROM deploy.job_results
    USING (
      SELECT ext_job_id
      FROM deploy.job_results
      WHERE recorded_at <= v_ts - i_age AND
      NOT EXISTS (
        SELECT 1 FROM deploy.jobs
        INNER JOIN deploy.commands USING (command_id)
        INNER JOIN deploy.minions USING (minion_id)
        WHERE jobs.ext_job_id = job_results.ext_job_id AND
              minions.fqdn = job_results.fqdn
      )
      LIMIT i_limit
    ) AS del_ids
    WHERE deploy.job_results.ext_job_id = del_ids.ext_job_id;

    GET DIAGNOSTICS v_deleted = ROW_COUNT;

    RETURN v_deleted;
END;
$$ LANGUAGE plpgsql;
