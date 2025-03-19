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
    clusters text[]
) AS $$
WITH dom0_info AS (
        SELECT fqdn, project, geo, cpu_cores, memory, ssd_space,
               sata_space, max_io, net_speed, allow_new_hosts
            FROM mdb.dom0_hosts
          WHERE fqdn = i_fqdn
    ),
    containers AS (
        SELECT coalesce(sum(cpu_guarantee), 0) AS guaranteed_cpus,
               coalesce(sum(memory_guarantee + hugetlb_limit), 0) AS memory_guarantee,
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
    )
SELECT d.cpu_cores AS total_cores,
       (d.cpu_cores - guaranteed_cpus)::int AS free_cores,

       pg_size_pretty(d.memory) AS total_memory_pretty,
       pg_size_pretty(d.memory - c.memory_guarantee) AS free_memory_pretty,
       pg_size_pretty(d.ssd_space) AS total_ssd_pretty,
       pg_size_pretty(d.ssd_space - coalesce(v.ssd_occupied, 0)) AS free_ssd_pretty,
       pg_size_pretty(d.sata_space) AS total_sata_pretty,
       pg_size_pretty(d.sata_space - coalesce(v.sata_occupied, 0)) AS free_sata_pretty,
       pg_size_pretty(d.max_io) AS total_io_pretty,
       pg_size_pretty(d.max_io - io_limit) AS free_io_pretty,
       pg_size_pretty(d.net_speed) AS total_net_pretty,
       pg_size_pretty(d.net_speed - net_guarantee) AS free_net_pretty,

       d.memory AS total_memory,
       (d.memory - c.memory_guarantee)::bigint AS free_memory,

       d.ssd_space AS total_ssd,
       (d.ssd_space - coalesce(v.ssd_occupied, 0))::bigint AS free_ssd,

       d.sata_space AS total_sata,
       (d.sata_space - coalesce(v.sata_occupied, 0))::bigint AS free_sata,

       d.max_io AS total_io,
       (d.max_io - io_limit)::bigint AS free_io,

       d.net_speed AS total_net,
       (d.net_speed - net_guarantee)::bigint AS free_net,

       clusters
    FROM containers c, dom0_info d, volumes v;
$$ LANGUAGE SQL STABLE STRICT;

CREATE MATERIALIZED VIEW mdb.dom0_info AS
    SELECT fqdn, project, geo, allow_new_hosts, di.*
      FROM mdb.dom0_hosts dh, LATERAL (
        SELECT (mdb.dom0_info(fqdn)).*) di;

CREATE UNIQUE INDEX uk_dom0_info ON mdb.dom0_info USING btree (fqdn);
