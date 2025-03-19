SELECT operation_id AS id,
       target_id,
       cluster_type,
       env AS environment,
       operation_type,
       created_by,
       created_at,
       started_at,
       modified_at,
       status,
       metadata,
       hidden,
       errors
  FROM code.add_finished_operation(
        i_operation_id     => %(task_id)s,
        i_cid              => %(cid)s,
        i_folder_id        => %(folder_id)s,
        i_operation_type   => %(operation_type)s,
        i_metadata         => %(metadata)s,
        i_user_id          => %(user_id)s,
        i_idempotence_data => %(idempotence_data)s::code.idempotence_data,
        i_version          => %(version)s,
        i_rev              => %(rev)s
)
