CREATE OR REPLACE FUNCTION code.get_job_result(
    i_job_result_id bigint
) RETURNS SETOF code.job_result AS $$
SELECT (code._as_job_result(job_results)).*
FROM deploy.job_results
WHERE job_result_id = i_job_result_id;
$$ LANGUAGE SQL STABLE;
