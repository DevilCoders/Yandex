CREATE INDEX CONCURRENTLY i_worker_queue_failed_acquire_count_task_id
    ON dbaas.worker_queue (failed_acquire_count, task_id)
 WHERE (failed_acquire_count > 0 AND result IS NULL);

CREATE INDEX CONCURRENTLY i_worker_queue_pending_not_delayed
    ON dbaas.worker_queue (failed_acquire_count, create_ts)
 WHERE delayed_until IS NULL
   AND start_ts IS NULL
   AND unmanaged = false;

DROP INDEX CONCURRENTLY dbaas.ip_worker_queue_pending_not_delayed;

CREATE INDEX CONCURRENTLY i_worker_queue_pending_and_delayed
    ON dbaas.worker_queue (failed_acquire_count, delayed_until)
 WHERE delayed_until IS NOT NULL
   AND start_ts IS NULL
   AND unmanaged = false;

DROP INDEX CONCURRENTLY dbaas.ip_worker_queue_pending_and_delayed;
