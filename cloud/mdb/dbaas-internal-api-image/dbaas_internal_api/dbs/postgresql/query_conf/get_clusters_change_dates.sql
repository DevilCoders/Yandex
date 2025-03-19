SELECT cd.*
  FROM unnest(CAST(%(cids)s AS text[])) AS i_cid,
       LATERAL(
           SELECT cid, create_ts AS changed_at
             FROM dbaas.worker_queue q
            WHERE q.cid = i_cid
              AND NOT q.hidden
              AND code.task_type_action(task_type) NOT IN (
                  'cluster-maintenance',
                  'cluster-resetup',
                  'cluster-offline-resetup')
            ORDER BY create_rev DESC,
                     acquire_rev NULLS LAST,
                     required_task_id NULLS FIRST
            FETCH FIRST ROW ONLY) cd
