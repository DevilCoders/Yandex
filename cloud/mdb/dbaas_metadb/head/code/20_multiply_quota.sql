-- Multiply quota by given multiplier
CREATE OR REPLACE FUNCTION code.multiply_quota(
    code.quota,
    int
) RETURNS code.quota AS $$
SELECT code.make_quota(
    i_cpu       => code.round_cpu_quota(($1.cpu * $2)::real),
    i_gpu       => $1.gpu * $2,
    i_memory    => $1.memory * $2,
    i_clusters  => $1.clusters * $2,
    i_ssd_space => $1.ssd_space * $2,
    i_hdd_space => $1.hdd_space * $2
);
$$ LANGUAGE SQL IMMUTABLE;
