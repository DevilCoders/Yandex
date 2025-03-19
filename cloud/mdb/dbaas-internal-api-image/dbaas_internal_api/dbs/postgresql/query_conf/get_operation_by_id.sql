SELECT operation_id AS id,
       target_id,
       cluster_type,
       env AS environment,
       operation_type,
       metadata,
       created_by,
       created_at,
       started_at,
       modified_at,
       status,
       errors,
       hidden
FROM code.get_operation_by_id(
    i_folder_id   => %(folder_id)s,
    i_operation_id => %(operation_id)s
)
