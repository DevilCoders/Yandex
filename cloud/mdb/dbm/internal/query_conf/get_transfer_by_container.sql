SELECT
    id,
    src_dom0,
    dest_dom0,
    container,
    placeholder
FROM
    mdb.transfers
WHERE
    container = %(fqdn)s
