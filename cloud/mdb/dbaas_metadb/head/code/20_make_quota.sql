CREATE OR REPLACE FUNCTION code.make_quota(
    i_cpu       real,
    i_memory    bigint,
    i_network   bigint DEFAULT 0,
    i_io        bigint DEFAULT 0,
    i_clusters  bigint DEFAULT 0,
    i_ssd_space bigint DEFAULT 0,
    i_hdd_space bigint DEFAULT 0,
    i_gpu       bigint DEFAULT 0
) RETURNS code.quota AS $$
SELECT (i_cpu, i_memory, i_network, i_io, i_ssd_space, i_hdd_space, i_clusters, i_gpu)::code.quota;
$$ LANGUAGE SQL IMMUTABLE;
