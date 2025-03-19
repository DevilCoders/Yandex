CREATE OR REPLACE FUNCTION code._as_job_result(
    job_result  deploy.job_results
) RETURNS code.job_result AS $$
SELECT $1.job_result_id,
       $1.order_id,
       $1.ext_job_id,
       $1.fqdn,
       $1.status::text,
       $1.result,
       $1.recorded_at;
$$ LANGUAGE SQL IMMUTABLE;
