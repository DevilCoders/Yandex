DELETE FROM mdb.volume_backups
WHERE
    dom0 = %(dom0)s
    AND dom0_path = %(dom0_path)s
    AND pending_delete = true
    AND delete_token = %(delete_token)s
RETURNING path
