CREATE OR REPLACE FUNCTION code.get_jobs(
    i_shipment_id       bigint,
    i_fqdn              text,
    i_ext_job_id        text,
    i_status            deploy.job_status,
    i_limit             bigint,
    i_last_job_id       bigint DEFAULT NULL,
    i_ascending         boolean DEFAULT true
)
RETURNS SETOF code.job AS $$
SELECT (code._as_job(jobs)).*
  FROM deploy.jobs
 JOIN deploy.commands
  USING (command_id)
 JOIN deploy.minions
  USING (minion_id)
 JOIN deploy.shipment_commands
  USING (shipment_command_id)
 JOIN deploy.shipments
  USING (shipment_id)
 WHERE (i_shipment_id IS NULL OR shipment_id = i_shipment_id)
  AND (i_fqdn IS NULL OR fqdn = i_fqdn)
  AND (i_ext_job_id IS NULL OR ext_job_id = i_ext_job_id)
  AND (i_status IS NULL OR jobs.status = i_status)
  AND (i_last_job_id IS NULL OR CASE WHEN i_ascending THEN job_id > i_last_job_id ELSE job_id < i_last_job_id END)
 ORDER BY job_id * CASE WHEN i_ascending THEN 1 ELSE -1 END ASC
 LIMIT i_limit;
$$ LANGUAGE SQL STABLE;
