SELECT task_id FROM dbaas.worker_queue
 WHERE version = %(version)s
   AND worker_id = %(worker_id)s
   AND end_ts IS NULL
   AND start_ts IS NOT NULL
   AND unmanaged = false
ORDER BY start_ts
