CREATE OR REPLACE FUNCTION code.get_operation_by_id(
    i_folder_id    bigint,
    i_operation_id text
) RETURNS SETOF code.operation AS $$
SELECT fmt.*
  FROM dbaas.worker_queue q
  JOIN dbaas.clusters c
 USING (cid),
       code.as_operation(q, c) fmt
 WHERE task_id = i_operation_id
   AND q.folder_id = i_folder_id;
$$ LANGUAGE SQL STABLE;
