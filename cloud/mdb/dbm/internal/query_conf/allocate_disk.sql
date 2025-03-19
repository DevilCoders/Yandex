UPDATE
    mdb.disks
SET
    has_data = true
WHERE
    id = %(id)s
