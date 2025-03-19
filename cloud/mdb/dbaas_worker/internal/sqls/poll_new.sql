SELECT q.task_id FROM dbaas.worker_queue q
 WHERE q.version = %(version)s
   AND q.start_ts IS NULL
   AND q.delayed_until IS NULL
   AND q.unmanaged = false
   AND q.failed_acquire_count < %(acquire_fail_limit)s
   AND q.cid NOT IN (
       SELECT cq.cid
         FROM dbaas.worker_queue cq
        WHERE cq.start_ts IS NOT NULL
          AND cq.end_ts IS NULL
          AND cq.unmanaged = false
          AND cq.cid IS NOT NULL)
   AND (q.required_task_id IS NULL OR
        q.required_task_id IN (SELECT rq.task_id
                                 FROM dbaas.worker_queue rq
                                WHERE rq.task_id = q.required_task_id
                                  AND rq.result = true))
ORDER BY q.failed_acquire_count, q.create_ts
LIMIT 1
FOR UPDATE
SKIP LOCKED
