SELECT
    dh.fqdn AS dom0
FROM
    mdb.dom0_hosts dh
    JOIN mdb.dom0_info di ON (dh.fqdn = di.fqdn)
WHERE
    dh.fqdn = %(fqdn)s
    AND di.generation IN %(generations)s
    AND di.free_cores >= %(cpu)s
    AND di.free_memory >= %(memory)s
    AND di.free_net >= %(net)s
    AND di.free_io >= %(io)s
    AND di.free_ssd >= %(ssd)s
    AND di.free_sata >= %(sata)s
    AND di.free_raw_disks >= %(raw_disks)s
    AND di.free_raw_disks_space >= %(raw_disks_space)s
FOR UPDATE SKIP LOCKED LIMIT 1
