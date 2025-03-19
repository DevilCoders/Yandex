SELECT
    job_id,
    cid,
    name,
    created_by,
    job_spec,
    create_ts,
    start_ts,
    end_ts,
    result,
    status,
    application_info
FROM
    dbaas.hadoop_jobs
WHERE
    job_id = %(job_id)s
    AND (%(cid)s IS NULL OR cid = %(cid)s)
