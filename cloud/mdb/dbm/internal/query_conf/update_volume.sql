UPDATE
    mdb.volumes
SET
    read_only = coalesce(%(read_only)s, read_only),
    space_guarantee = coalesce(%(space_guarantee)s, space_guarantee),
    space_limit = coalesce(%(space_limit)s, space_limit),
    inode_guarantee = coalesce(%(inode_guarantee)s, inode_guarantee),
    inode_limit = coalesce(%(inode_limit)s, inode_limit)
WHERE
    container = %(container)s
    AND path = %(path)s
RETURNING
    dom0
