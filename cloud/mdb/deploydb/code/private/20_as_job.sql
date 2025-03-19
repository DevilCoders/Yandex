CREATE OR REPLACE FUNCTION code._as_job(
    job   deploy.jobs
) RETURNS code.job AS $$
SELECT $1.job_id,
       $1.ext_job_id,
       $1.command_id,
       $1.status::text,
       $1.created_at,
       $1.updated_at;
$$ LANGUAGE SQL IMMUTABLE;
