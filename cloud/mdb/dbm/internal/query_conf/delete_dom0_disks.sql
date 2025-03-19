DELETE FROM
    mdb.disks
WHERE
    dom0 = %(dom0)s
    AND id in %(ids)s
