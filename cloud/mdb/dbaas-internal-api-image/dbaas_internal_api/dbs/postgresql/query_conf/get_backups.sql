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
WHERE (%(cid)s IS NULL OR %(cid)s = cid)
  AND (%(subcid)s IS NULL OR %(subcid)s = subcid)
  AND (%(shard_id)s IS NULL OR %(shard_id)s = shard_id)
  AND (%(backup_statuses)s IS NULL OR status = ANY(%(backup_statuses)s::dbaas.backup_status[]))
ORDER BY backups.finished_at DESC, backups.created_at DESC
