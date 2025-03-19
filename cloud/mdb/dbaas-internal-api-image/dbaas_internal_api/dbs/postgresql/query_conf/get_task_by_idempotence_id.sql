SELECT operation_id AS id,
       request_hash
  FROM code.get_operation_id_by_idempotence(
    i_idempotence_id => %(idempotence_id)s,
    i_folder_id      => %(folder_id)s,
    i_user_id        => %(user_id)s
  )
