UPDATE
    mdb.containers
SET
    dom0 = %(dest_dom0)s
WHERE
    fqdn = %(container)s
    AND dom0 = %(src_dom0)s
