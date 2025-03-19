UPDATE
    mdb.containers
SET
    pending_delete = true,
    delete_token = %(token)s
WHERE
    fqdn = %(fqdn)s
RETURNING dom0
