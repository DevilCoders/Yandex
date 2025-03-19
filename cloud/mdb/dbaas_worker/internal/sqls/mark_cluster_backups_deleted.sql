UPDATE dbaas.backups
SET status= 'DELETED',
    updated_at = now(),
    finished_at = now()
WHERE cid = %(cid)s
