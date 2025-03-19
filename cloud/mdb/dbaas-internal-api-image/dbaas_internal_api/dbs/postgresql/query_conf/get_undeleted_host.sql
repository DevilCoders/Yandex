SELECT
    task_args->'host' AS host
FROM
    dbaas.worker_queue
WHERE
    task_type like '%%_host_delete'
    AND cid = %(cid)s
    AND result = false
