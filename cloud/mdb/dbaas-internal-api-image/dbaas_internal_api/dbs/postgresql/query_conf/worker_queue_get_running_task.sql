SELECT task_id as operation_id
  FROM dbaas.worker_queue
 WHERE result is null
   AND cid = %(cid)s
   AND unmanaged is false