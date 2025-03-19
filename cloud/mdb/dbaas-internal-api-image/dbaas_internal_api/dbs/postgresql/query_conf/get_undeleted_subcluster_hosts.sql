SELECT
    task_args->'host_list' AS host_list
FROM
    dbaas.worker_queue
WHERE
    task_type like '%%_subcluster_delete'
    AND cid = %(cid)s
    AND result = false
