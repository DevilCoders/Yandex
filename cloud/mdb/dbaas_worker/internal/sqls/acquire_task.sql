SELECT task_id,
       cid,
       extract(epoch FROM timeout) as timeout,
       task_type,
       task_args,
       coalesce(context, '{}'::jsonb) as context,
       restart_count,
       created_by,
       create_ts,
       ext_folder_id AS folder_id,
       coalesce(task_args->'feature_flags', '[]'::jsonb) as feature_flags,
       tracing,
       c.status AS cluster_status,
       c.network_id
  FROM code.acquire_task(
      i_worker_id => %(worker_id)s,
      i_task_id   => %(task_id)s
) t JOIN dbaas.clusters c USING (cid)
