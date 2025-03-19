UPDATE
    mdb.dom0_hosts
SET
    allow_new_hosts = %(allow_new_hosts)s,
    allow_new_hosts_updated_by = %(allow_new_hosts_updated_by)s
WHERE
    fqdn = %(fqdn)s
RETURNING
    allow_new_hosts, allow_new_hosts_updated_by
