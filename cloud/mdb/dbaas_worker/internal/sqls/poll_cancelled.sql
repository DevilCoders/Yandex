SELECT task_id FROM dbaas.worker_queue
 WHERE task_id in %(task_ids)s
   AND cancelled;
