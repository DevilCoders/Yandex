UPDATE
  dbaas.worker_queue
SET
  delayed_until = now() + (%(interval)s || ' seconds')::interval
WHERE
  task_id = %(task_id)s
