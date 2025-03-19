SELECT
    container,
    path,
    dom0,
    dom0_path,
    create_ts,
    space_limit,
    disk_id,
    pending_delete
FROM
    mdb.volume_backups
WHERE
    (%(container)s IS NULL OR container ~ %(container)s)
    AND (%(path)s IS NULL OR path ~ %(path)s)
    AND (%(dom0)s IS NULL OR dom0 ~ %(dom0)s)
    AND (%(dom0_path)s IS NULL OR dom0_path ~ %(dom0_path)s)
    AND (%(disk_id)s IS NULL OR disk_id::text ~ %(disk_id)s)
ORDER BY
    container, path
LIMIT
    %(limit)s
OFFSET
    %(offset)s
