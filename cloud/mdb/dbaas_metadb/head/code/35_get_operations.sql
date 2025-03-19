CREATE OR REPLACE FUNCTION code.get_operations(
    i_folder_id             bigint,
    i_cid                   text,
    i_limit                 integer,
    i_env                   dbaas.env_type            DEFAULT NULL,
    i_cluster_type          dbaas.cluster_type        DEFAULT NULL,
    i_type                  text                      DEFAULT NULL,
    i_created_by            text                      DEFAULT NULL,
    i_page_token_id         text                      DEFAULT NULL,
    i_page_token_create_ts  timestamp with time zone  DEFAULT NULL,
    i_include_hidden        boolean                   DEFAULT NULL
) RETURNS SETOF code.operation AS $$
SELECT fmt.*
  FROM dbaas.worker_queue q
  JOIN dbaas.clusters c
 USING (cid),
       code.as_operation(q, c) fmt
 WHERE (i_cid IS NULL OR c.cid = i_cid)
   AND (i_folder_id IS NULL OR q.folder_id = i_folder_id)
   AND (i_env IS NULL OR c.env = i_env)
   AND (i_cluster_type IS NULL OR c.type = i_cluster_type)
   AND (i_type IS NULL OR q.task_type = i_type)
   AND (i_created_by IS NULL OR q.created_by = i_created_by)
   AND (
        (i_page_token_create_ts IS NULL AND i_page_token_id IS NULL)
        OR (q.create_ts, q.task_id) < (i_page_token_create_ts, i_page_token_id)
   )
   AND (i_include_hidden = true OR q.hidden = false)
 ORDER BY q.create_ts DESC, q.task_id DESC
 LIMIT i_limit;
$$ LANGUAGE SQL STABLE;
