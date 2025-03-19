INSERT INTO mdb.dom0_hosts (
    fqdn, project, geo, cpu_cores, memory,
    ssd_space, sata_space, max_io, net_speed,
    heartbeat, generation, switch
) VALUES (
    %(fqdn)s, %(project)s, %(geo)s, %(cpu_cores)s, %(memory)s,
    %(ssd_space)s, %(sata_space)s, %(max_io)s, %(net_speed)s,
    current_timestamp, %(generation)s, %(switch)s
)
ON CONFLICT (fqdn) DO UPDATE
SET project = %(project)s,
    geo = %(geo)s,
    cpu_cores = %(cpu_cores)s,
    memory = %(memory)s,
    ssd_space = %(ssd_space)s,
    sata_space = %(sata_space)s,
    max_io = %(max_io)s,
    net_speed = %(net_speed)s,
    heartbeat = current_timestamp,
    generation = %(generation)s,
    switch = %(switch)s
