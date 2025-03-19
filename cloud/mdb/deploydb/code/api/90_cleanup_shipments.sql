CREATE OR REPLACE FUNCTION code.cleanup_shipments(
    i_age interval,
    i_limit bigint
) RETURNS bigint AS $$
DECLARE
    v_ts                  timestamptz;
    v_deleted             bigint;
BEGIN
    v_ts := clock_timestamp();

    WITH deleted_shipements AS (
      DELETE FROM deploy.shipments
      WHERE shipment_id IN (
        SELECT shipment_id
        FROM deploy.shipments
        WHERE status != 'INPROGRESS' AND updated_at  <= v_ts - i_age
        LIMIT i_limit
      )

      RETURNING *
    ), deleted_shipment_command_ids AS (
      DELETE FROM deploy.shipment_commands
      WHERE shipment_id IN (SELECT shipment_id FROM deleted_shipements)
      RETURNING shipment_command_id
    ), deleted_job_result_ids AS (
      DELETE FROM deploy.job_results
      USING(
          SELECT
            deploy.jobs.ext_job_id AS ext_job_id,
            deploy.minions.fqdn AS fqdn
          FROM deploy.jobs
          INNER JOIN deploy.commands USING(command_id)
          INNER JOIN deploy.minions USING(minion_id)
          WHERE deploy.commands.shipment_command_id IN (
            SELECT shipment_command_id FROM deleted_shipment_command_ids
          )
      ) AS job_result_to_delete
      WHERE job_result_to_delete.ext_job_id = deploy.job_results.ext_job_id AND
            job_result_to_delete.fqdn = deploy.job_results.fqdn
      RETURNING job_result_id
    )
    SELECT COUNT(*) INTO v_deleted
    FROM deleted_shipements;

    RETURN v_deleted;
END;
$$ LANGUAGE plpgsql;
