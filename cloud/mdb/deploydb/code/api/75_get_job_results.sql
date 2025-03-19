CREATE OR REPLACE FUNCTION code.get_job_results(
  i_ext_job_id          text,
  i_fqdn                text,
  i_status              deploy.job_result_status,
  i_limit               bigint,
  i_last_job_result_id  bigint DEFAULT NULL,
  i_ascending           boolean DEFAULT true
) RETURNS SETOF code.job_result AS $$
SELECT (code._as_job_result(job_results)).*
  FROM deploy.job_results
  WHERE (i_ext_job_id IS NULL OR ext_job_id = i_ext_job_id)
    AND (i_fqdn IS NULL OR fqdn = i_fqdn)
    AND (i_status IS NULL OR status = i_status)
    AND (i_last_job_result_id IS NULL
      OR CASE WHEN i_ascending THEN job_result_id > i_last_job_result_id ELSE job_result_id < i_last_job_result_id END)
  ORDER BY job_result_id * CASE WHEN i_ascending THEN 1 ELSE -1 END ASC
  LIMIT i_limit;
$$ LANGUAGE SQL STABLE;
