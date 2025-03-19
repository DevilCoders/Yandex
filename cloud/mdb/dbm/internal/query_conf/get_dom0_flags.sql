SELECT
    use_vlan688
FROM mdb.dom0_hosts
WHERE fqdn = %(fqdn)s
