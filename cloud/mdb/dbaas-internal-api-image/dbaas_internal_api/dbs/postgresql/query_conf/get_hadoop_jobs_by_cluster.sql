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
    cid = %(cid)s
    AND (%(statuses)s IS NULL OR status = ANY(%(statuses)s::dbaas.hadoop_jobs_status[]))
    AND (%(page_token_create_ts)s IS NULL OR create_ts <= %(page_token_create_ts)s)
    AND (%(page_token_job_id)s IS NULL OR job_id < %(page_token_job_id)s)
ORDER BY
    create_ts DESC, job_id DESC
LIMIT %(limit)s
