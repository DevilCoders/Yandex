CREATE OR REPLACE FUNCTION code.get_rev_pg_cluster_nodes(
    i_fqdn      text,
    i_rev       bigint
) RETURNS jsonb AS $$
SELECT
    jsonb_strip_nulls(jsonb_build_object('ha', jsonb_agg(node_fqdn) FILTER (WHERE replication_source_fqdn IS NULL),
        'cascade_replicas', jsonb_agg(node_fqdn) FILTER (WHERE replication_source_fqdn = i_fqdn)))
FROM (
    SELECT hosts_revs.fqdn AS node_fqdn, value->'data'->'pgsync'->>'replication_source' replication_source_fqdn FROM dbaas.hosts_revs
        LEFT JOIN dbaas.pillar_revs USING (fqdn)
        WHERE hosts_revs.subcid = (SELECT subcid FROM dbaas.hosts_revs WHERE fqdn = i_fqdn AND rev = i_rev)
          AND hosts_revs.rev = i_rev
          AND pillar_revs.rev = i_rev
        ORDER BY 1
) x;
$$ LANGUAGE SQL STABLE;
