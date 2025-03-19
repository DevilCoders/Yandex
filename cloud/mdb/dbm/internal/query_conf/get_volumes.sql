SELECT
    v.container,
    v.path,
    v.dom0,
    v.dom0_path,
    v.backend,
    v.space_guarantee,
    v.space_limit,
    v.inode_guarantee,
    v.inode_limit,
    v.read_only,
    vb.path IS NOT NULL AS pending_backup
FROM
    mdb.volumes v
LEFT JOIN
    mdb.volume_backups vb ON (v.container = vb.container AND v.dom0 = vb.dom0 AND v.path = vb.path)
WHERE
    (%(container)s IS NULL OR v.container ~ %(container)s)
    AND (%(path)s IS NULL OR v.path ~ %(path)s)
    AND (%(dom0)s IS NULL OR v.dom0 ~ %(dom0)s)
    AND (%(dom0_path)s IS NULL OR v.dom0_path ~ %(dom0_path)s)
    AND (%(backend)s IS NULL OR v.backend = %(backend)s)
    AND (%(space_guarantee)s IS NULL OR v.space_guarantee = %(space_guarantee)s)
    AND (%(space_limit)s IS NULL OR v.space_limit = %(space_limit)s)
    AND (%(inode_guarantee)s IS NULL OR v.inode_guarantee = %(inode_guarantee)s)
    AND (%(inode_limit)s IS NULL OR v.inode_limit = %(inode_limit)s)
    AND (%(read_only)s IS NULL OR v.read_only = %(read_only)s)
ORDER BY
    container,
    path
LIMIT
    %(limit)s
OFFSET
    %(offset)s
