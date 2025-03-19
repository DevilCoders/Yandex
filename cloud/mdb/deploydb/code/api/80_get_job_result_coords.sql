CREATE OR REPLACE FUNCTION code.get_job_result_coords(
  i_ext_job_id  text,
  i_fqdn        text
) RETURNS SETOF code.job_result_coords AS $$
SELECT shipments.shipment_id,
       commands.command_id,
       jobs.job_id,
       shipments.tracing
  FROM deploy.shipments
  JOIN deploy.shipment_commands
 USING (shipment_id)
  JOIN deploy.commands
 USING (shipment_command_id)
  JOIN deploy.jobs
 USING (command_id)
  JOIN deploy.minions
 USING (minion_id)
 WHERE jobs.ext_job_id = i_ext_job_id AND minions.fqdn = i_fqdn;
$$ LANGUAGE SQL STABLE;
