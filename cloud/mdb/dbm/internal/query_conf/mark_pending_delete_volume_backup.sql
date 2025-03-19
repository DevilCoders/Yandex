UPDATE mdb.volume_backups
SET pending_delete = true
WHERE
    container = %(container)s
    AND path = %(path)s
    AND dom0 = %(dom0)s
    AND (%(delete_token)s IS NULL OR delete_token = %(delete_token)s)
RETURNING dom0_path, delete_token
