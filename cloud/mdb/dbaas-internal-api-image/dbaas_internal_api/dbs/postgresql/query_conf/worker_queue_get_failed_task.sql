  SELECT task_id AS operation_id
    FROM dbaas.worker_queue q
    JOIN dbaas.clusters c ON (q.cid = c.cid)
   WHERE c.status::text LIKE '%%ERROR'
     AND q.result = false
     AND q.unmanaged = false
     AND c.cid = %(cid)s
ORDER BY q.end_ts ASC
   LIMIT 1;
