DELETE FROM
    mdb.volumes
WHERE
    container = %(fqdn)s
