SELECT local_id,
       mount_point
FROM dbaas.disks d
JOIN dbaas.disk_placement_groups p
    USING (pg_id, cid)
WHERE fqdn = %(fqdn)s
