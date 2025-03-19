INSERT INTO dbaas.idempotence (
    idempotence_id,
    task_id,
    folder_id,
    user_id,
    request_hash
) VALUES (
    %(idempotence_id)s,
    %(task_id)s,
    %(folder_id)s,
    %(user_id)s,
    %(request_hash)s
)
