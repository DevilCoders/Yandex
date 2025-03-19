INSERT INTO mdb.volume_backups (
    container,
    path,
    dom0,
    dom0_path,
    space_limit,
    disk_id
) VALUES (
    %(container)s,
    %(path)s,
    %(dom0)s,
    %(dom0_path)s,
    %(space_limit)s,
    %(disk_id)s
)
