CREATE OR REPLACE FUNCTION code.unquoted_clusters()
RETURNS dbaas.cluster_type[] AS $$
SELECT ARRAY['hadoop_cluster'::dbaas.cluster_type];
$$ LANGUAGE SQL IMMUTABLE;
