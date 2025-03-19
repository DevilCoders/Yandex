-- Create code.quota from flavor
CREATE OR REPLACE FUNCTION code.flavor_as_quota(
    i_flavor_name text,
    i_ssd_space   bigint DEFAULT NULL,
    i_hdd_space   bigint DEFAULT NULL
) RETURNS SETOF code.quota AS $$
 SELECT q.*
   FROM dbaas.flavors,
LATERAL code.make_quota(
            i_cpu       => cpu_guarantee,
            i_gpu       => gpu_limit,
            i_memory    => memory_guarantee,
            i_clusters  => NULL,
            i_ssd_space => i_ssd_space,
            i_hdd_space => i_hdd_space
        ) q
  WHERE flavors.name = i_flavor_name;
$$ LANGUAGE SQL STABLE;
