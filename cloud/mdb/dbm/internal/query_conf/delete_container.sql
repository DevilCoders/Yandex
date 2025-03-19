DELETE FROM
    mdb.containers
WHERE
    fqdn = %(fqdn)s
    AND delete_token = %(token)s
RETURNING
    fqdn
