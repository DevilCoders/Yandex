CREATE OR REPLACE FUNCTION code._get_job(
    i_ext_job_id text,
    i_fqdn text
)
RETURNS deploy.jobs AS $$
SELECT jobs
  FROM deploy.jobs
  JOIN deploy.commands
 USING (command_id)
  JOIN deploy.minions
 USING (minion_id)
 WHERE jobs.ext_job_id = i_ext_job_id AND minions.fqdn = i_fqdn;
$$ LANGUAGE SQL STABLE;