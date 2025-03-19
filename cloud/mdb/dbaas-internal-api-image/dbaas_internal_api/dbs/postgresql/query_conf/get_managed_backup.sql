SELECT
    backup_id,
    cid,
    subcid,
    shard_id,
    method,
    initiator,
    status,
    metadata,
    started_at,
    finished_at
FROM dbaas.backups
WHERE backup_id = %(bid)s
