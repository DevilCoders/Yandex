CREATE OR REPLACE FUNCTION code.get_folder_id_by_operation(
    i_operation_id text
) RETURNS bigint AS $$
SELECT folder_id
  FROM dbaas.worker_queue
 WHERE task_id = i_operation_id;
$$ LANGUAGE SQL STABLE;
