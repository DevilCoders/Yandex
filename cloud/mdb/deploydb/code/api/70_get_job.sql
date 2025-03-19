CREATE OR REPLACE FUNCTION code.get_job(
    i_job_id bigint
)
RETURNS SETOF code.job AS $$
SELECT (code._as_job(jobs)).*
  FROM deploy.jobs
 WHERE job_id = i_job_id;
$$ LANGUAGE SQL STABLE;
