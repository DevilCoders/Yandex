INSERT INTO mdb.disks (
    id,
    max_space_limit,
    has_data,
    dom0
) VALUES (
    %(id)s,
    %(max_space_limit)s,
    %(has_data)s,
    %(dom0)s
) ON CONFLICT (id) DO UPDATE
SET max_space_limit = %(max_space_limit)s,
    has_data = %(has_data)s,
    dom0 = %(dom0)s
