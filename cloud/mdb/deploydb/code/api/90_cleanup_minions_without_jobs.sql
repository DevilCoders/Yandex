CREATE OR REPLACE FUNCTION code.cleanup_minions_without_jobs(
    i_age interval,
    i_limit bigint
) RETURNS bigint AS $$
DECLARE
    v_ts                  timestamptz;
    v_deleted             bigint;
BEGIN
    v_ts := clock_timestamp();

    WITH deleted_minions AS (
      DELETE FROM deploy.minions
      WHERE minion_id IN (
        SELECT minion_id FROM deploy.minions
        WHERE updated_at  <= v_ts - i_age
              AND deleted
              AND NOT EXISTS (
                SELECT 1
                FROM deploy.commands
                WHERE minions.minion_id = commands.minion_id
              )
              AND NOT EXISTS (
                SELECT 1
                FROM deploy.job_results
                WHERE minions.fqdn = job_results.fqdn
              )
        LIMIT i_limit
      )
      RETURNING *
    ), deleted_changes AS (
      DELETE FROM deploy.minions_change_log
      WHERE deploy.minions_change_log.minion_id IN (
        SELECT deleted_minions.minion_id FROM deleted_minions
      )
    )
    SELECT COUNT(*) INTO v_deleted
    FROM deleted_minions;

    RETURN v_deleted;
END;
$$ LANGUAGE plpgsql;
