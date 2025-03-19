SELECT
    v.container,
    v.path,
    v.dom0,
    v.dom0_path,
    v.backend,
    v.disk_id,
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
    v.container = %(container)s
ORDER BY
    path
