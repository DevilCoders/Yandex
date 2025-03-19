SELECT
    dh.fqdn AS dom0
FROM
    mdb.dom0_hosts dh
    JOIN mdb.dom0_info di ON (dh.fqdn = di.fqdn)
    JOIN mdb.reserved_resources r ON (dh.generation = r.generation)
WHERE
    di.project = %(project)s
    AND di.geo = %(geo)s
    AND di.heartbeat >= current_timestamp - '5 minutes'::interval
    AND di.generation IN %(generations)s
    AND di.allow_new_hosts
    AND di.free_cores >= r.cpu_cores + %(cpu)s
    AND di.free_memory >= r.memory + %(memory)s::bigint
    AND di.free_net >= r.net + %(net)s::bigint
    AND di.free_io >= r.io + %(io)s::bigint
    AND di.free_ssd >= %(ssd)s::bigint
    AND di.free_sata >= %(sata)s::bigint
    AND di.free_raw_disks >= %(raw_disks)s::bigint
    AND di.free_raw_disks_space >= %(raw_disks_space)s::bigint
    AND (di.free_raw_disks_space / greatest(di.free_raw_disks, 1)) >= %(max_raw_disk_space)s::bigint
    AND (%(cluster_name)s IS NULL OR di.fqdn NOT IN (
        SELECT dom0 FROM mdb.containers
         WHERE cluster_name = %(cluster_name)s
    ))
    AND (((NOT %(switch_aware)s) OR %(cluster_name)s IS NULL) OR di.switch NOT IN (
        SELECT d.switch FROM mdb.containers c JOIN mdb.dom0_hosts d ON (c.dom0 = d.fqdn)
         WHERE c.cluster_name = %(cluster_name)s
    ))
ORDER BY
    di.generation,
    di.free_cores           DESC,
    di.free_io              DESC,
    di.free_memory          DESC,
    di.free_net             DESC,
    di.free_ssd             DESC,
    di.free_sata            DESC,
    di.free_raw_disks       DESC,
    di.free_raw_disks_space DESC,
    di.fqdn
FOR UPDATE SKIP LOCKED LIMIT 1
