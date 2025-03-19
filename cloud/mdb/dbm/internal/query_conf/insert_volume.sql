INSERT INTO mdb.volumes (
    container, path, dom0, dom0_path, backend,
    read_only, space_guarantee, space_limit,
    inode_guarantee, inode_limit, disk_id)
VALUES (
    %(container)s, %(path)s, %(dom0)s, %(dom0_path)s, %(backend)s,
    %(read_only)s, %(space_guarantee)s, %(space_limit)s,
    %(inode_guarantee)s, %(inode_limit)s, %(disk_id)s)
