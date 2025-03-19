SELECT task_id AS operation_id
  FROM dbaas.worker_queue
 WHERE task_type='hadoop_job_create'
   AND cid=%(cid)s
   AND unmanaged = true
   AND metadata->>'jobId' = %(job_id)s;