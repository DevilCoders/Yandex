CREATE OR REPLACE FUNCTION code.add_hadoop_job(
    i_job_id     text,
    i_cid        text,
    i_name       text,
    i_created_by text,
    i_job_spec   jsonb
) RETURNS TABLE (job_id text, cid text, name text, created_by text, job_spec jsonb) AS $$
BEGIN
    INSERT INTO dbaas.hadoop_jobs (
        job_id, cid, name, created_by, job_spec
    ) VALUES (
        i_job_id, i_cid, i_name, i_created_by, i_job_spec
    );

    RETURN QUERY SELECT i_job_id, i_cid, i_name, i_created_by, i_job_spec;
END;
$$ LANGUAGE plpgsql;
