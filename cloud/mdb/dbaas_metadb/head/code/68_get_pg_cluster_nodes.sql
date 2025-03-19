CREATE OR REPLACE FUNCTION code.get_pg_cluster_nodes(
    i_fqdn      text
) RETURNS jsonb AS $$
SELECT
    jsonb_strip_nulls(jsonb_build_object('ha', jsonb_agg(node_fqdn) FILTER (WHERE replication_source_fqdn IS NULL),
        'cascade_replicas', jsonb_agg(node_fqdn) FILTER (WHERE replication_source_fqdn = i_fqdn)))
FROM (
    SELECT hosts.fqdn AS node_fqdn, value->'data'->'pgsync'->>'replication_source' replication_source_fqdn FROM dbaas.hosts
        LEFT JOIN dbaas.pillar USING (fqdn)
        WHERE hosts.subcid = (SELECT subcid FROM dbaas.hosts WHERE fqdn = i_fqdn)
        ORDER BY 1
) x;
$$ LANGUAGE SQL STABLE;
