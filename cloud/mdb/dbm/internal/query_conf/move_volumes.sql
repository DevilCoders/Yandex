UPDATE
    mdb.volumes
SET
    dom0 = %(dest_dom0)s
WHERE
    container = %(container)s
    AND dom0 = %(src_dom0)s
