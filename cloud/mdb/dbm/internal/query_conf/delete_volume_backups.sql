DELETE FROM mdb.volume_backups
WHERE
    container = %(container)s
    AND dom0 = %(dom0)s
    AND (%(path)s IS NULL OR path = %(path)s)
