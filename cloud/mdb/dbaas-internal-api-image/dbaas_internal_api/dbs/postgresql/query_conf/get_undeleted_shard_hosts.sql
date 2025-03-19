SELECT
    task_args->'shard_hosts' AS shard_hosts
FROM
    dbaas.worker_queue
WHERE
    task_type like '%%_shard_delete'
    AND cid = %(cid)s
    AND result = false
