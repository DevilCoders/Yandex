CREATE OR REPLACE FUNCTION code.visible(
    dbaas.clusters
) RETURNS bool AS $$
SELECT dbaas.visible_cluster_status($1.status);
$$ LANGUAGE SQL IMMUTABLE;
