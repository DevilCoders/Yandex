SELECT
    id,
    max_space_limit,
    has_data,
    dom0
FROM mdb.disks
WHERE dom0 = %(fqdn)s
