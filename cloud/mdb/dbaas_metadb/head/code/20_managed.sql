CREATE OR REPLACE FUNCTION code.managed(
    dbaas.clusters
) RETURNS bool AS $$
SELECT $1.type NOT IN ('hadoop_cluster'::dbaas.cluster_type);
$$ LANGUAGE SQL IMMUTABLE;
