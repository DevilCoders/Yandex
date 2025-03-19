CREATE OR REPLACE FUNCTION code.get_operation_id_by_idempotence(
    i_idempotence_id uuid,
    i_folder_id      bigint,
    i_user_id        text
) RETURNS SETOF code.operation_id_by_idempotence AS $$
SELECT task_id, request_hash
  FROM dbaas.idempotence
 WHERE idempotence_id = i_idempotence_id
   AND folder_id = i_folder_id
   AND user_id = i_user_id;
$$ LANGUAGE SQL STABLE;
