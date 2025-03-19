DROP VIEW mdb.dom0_info;

DROP FUNCTION mdb.dom0_info(text);

CREATE OR REPLACE FUNCTION mdb.dom0_info(
    i_fqdn text
) RETURNS TABLE (
    total_cores integer,
    free_cores integer,
    total_memory_pretty text,
    free_memory_pretty text,
    total_ssd_pretty text,
    free_ssd_pretty text,
    total_sata_pretty text,
    free_sata_pretty text,
    total_io_pretty text,
    free_io_pretty text,
    total_net_pretty text,
    free_net_pretty text,
    total_raw_disks_space_pretty text,
    free_raw_disks_space_pretty text,
    total_memory bigint,
    free_memory bigint,
    total_ssd bigint,
    free_ssd bigint,
    total_sata bigint,
    free_sata bigint,
    total_io bigint,
    free_io bigint,
    total_net bigint,
    free_net bigint,
    total_raw_disks_space bigint,
    free_raw_disks_space bigint,
    total_raw_disks bigint,
    free_raw_disks bigint,
    clusters text[],
    switch text
) AS $$
WITH dom0_info AS (
        SELECT fqdn, project, geo, cpu_cores, memory, ssd_space,
               sata_space, max_io, net_speed, switch,
               allow_new_hosts, allow_new_hosts_updated_by
            FROM mdb.dom0_hosts
          WHERE fqdn = i_fqdn
    ),
    containers AS (
        SELECT coalesce(sum(cpu_guarantee), 0) AS guaranteed_cpus,
               coalesce(sum(coalesce(memory_guarantee, 0) + coalesce(hugetlb_limit, 0)), 0) AS memory_guarantee,
               coalesce(sum(io_limit), 0) AS io_limit,
               coalesce(sum(net_guarantee), 0) AS net_guarantee,
               array_agg(cluster_name ORDER BY cluster_name) AS clusters
            FROM mdb.containers
          WHERE dom0 = i_fqdn
    ),
    volumes AS (
        SELECT sum(coalesce(space_guarantee, space_limit, 0)) FILTER
                   (WHERE dom0_path ~ '^/data/') AS ssd_occupied,
               sum(coalesce(space_guarantee, space_limit, 0)) FILTER
                   (WHERE dom0_path ~ '^/slow/') AS sata_occupied
            FROM mdb.volumes
          WHERE dom0 = i_fqdn
    ),
    raw_disks AS (
        SELECT min(max_space_limit) AS max_space_limit,
               count(*) FILTER (WHERE NOT has_data) as unused_raw_disks,
               count(*) AS total_raw_disks
          FROM mdb.disks
         WHERE dom0 = i_fqdn
    ),
    volume_backups AS (
        SELECT sum(coalesce(space_limit, 0)) FILTER
                   (WHERE dom0_path ~ '^/data/') AS ssd_occupied,
               sum(coalesce(space_limit, 0)) FILTER
                   (WHERE dom0_path ~ '^/slow/') AS sata_occupied
            FROM mdb.volume_backups
          WHERE dom0 = i_fqdn
    )
SELECT d.cpu_cores AS total_cores,
       (d.cpu_cores - guaranteed_cpus)::int AS free_cores,

       pg_size_pretty(d.memory) AS total_memory_pretty,
       pg_size_pretty(d.memory - c.memory_guarantee) AS free_memory_pretty,
       pg_size_pretty(d.ssd_space) AS total_ssd_pretty,
       pg_size_pretty(d.ssd_space - coalesce(v.ssd_occupied, 0) - coalesce(b.ssd_occupied, 0)) AS free_ssd_pretty,
       pg_size_pretty(d.sata_space) AS total_sata_pretty,
       pg_size_pretty(d.sata_space - coalesce(v.sata_occupied, 0) - coalesce(b.sata_occupied, 0)) AS free_sata_pretty,
       pg_size_pretty(d.max_io) AS total_io_pretty,
       pg_size_pretty(d.max_io - io_limit) AS free_io_pretty,
       pg_size_pretty(d.net_speed) AS total_net_pretty,
       pg_size_pretty(d.net_speed - net_guarantee) AS free_net_pretty,
       pg_size_pretty(coalesce(r.max_space_limit, 0) * r.total_raw_disks) AS total_raw_disks_space_pretty,
       pg_size_pretty(coalesce(r.max_space_limit, 0) * r.unused_raw_disks) AS free_raw_disks_space_pretty,

       d.memory AS total_memory,
       (d.memory - c.memory_guarantee)::bigint AS free_memory,

       d.ssd_space AS total_ssd,
       (d.ssd_space - coalesce(v.ssd_occupied, 0)::bigint - coalesce(b.ssd_occupied, 0))::bigint AS free_ssd,

       d.sata_space AS total_sata,
       (d.sata_space - coalesce(v.sata_occupied, 0)::bigint - coalesce(b.sata_occupied, 0))::bigint AS free_sata,

       d.max_io AS total_io,
       (d.max_io - io_limit)::bigint AS free_io,

       d.net_speed AS total_net,
       (d.net_speed - net_guarantee)::bigint AS free_net,

       coalesce(r.max_space_limit, 0) * r.total_raw_disks AS total_raw_disks_space,
       coalesce(r.max_space_limit, 0) * r.unused_raw_disks AS free_raw_disks_space,
       r.total_raw_disks AS total_raw_disks,
       r.unused_raw_disks AS free_raw_disks,

       clusters,
       switch
    FROM containers c, dom0_info d, volumes v, raw_disks r, volume_backups b;
$$ LANGUAGE SQL STABLE STRICT;

CREATE VIEW mdb.dom0_info AS
    SELECT fqdn, project, geo, allow_new_hosts, allow_new_hosts_updated_by, generation, heartbeat, di.*
      FROM mdb.dom0_hosts dh, LATERAL (
        SELECT (mdb.dom0_info(fqdn)).*) di;
