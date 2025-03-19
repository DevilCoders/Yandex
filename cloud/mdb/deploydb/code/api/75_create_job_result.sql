CREATE OR REPLACE FUNCTION code.create_job_result(
    i_ext_job_id    text,
    i_fqdn          text,
    i_status        deploy.job_result_status,
    i_result        jsonb,
    i_ts            timestamptz default CURRENT_TIMESTAMP
) RETURNS code.job_result AS $$
SELECT * FROM code._create_job_result(
    i_ext_job_id    => i_ext_job_id,
    i_fqdn          => i_fqdn,
    i_status        => i_status,
    i_result        => i_result,
    i_ts            => i_ts);
$$ LANGUAGE SQL VOLATILE;
