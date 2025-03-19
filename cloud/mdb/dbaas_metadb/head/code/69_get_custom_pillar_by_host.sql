CREATE OR REPLACE FUNCTION code.get_custom_pillar_by_host(
    i_fqdn      text
) RETURNS jsonb AS $$
DECLARE
    v_ctype dbaas.cluster_type;
BEGIN
    SELECT type INTO v_ctype
        FROM dbaas.clusters
        JOIN dbaas.subclusters USING (cid)
        JOIN dbaas.hosts USING (subcid)
        WHERE fqdn = i_fqdn;
    IF v_ctype = 'postgresql_cluster' THEN
        RETURN concat('{"data": {"cluster_nodes":', (SELECT code.get_pg_cluster_nodes(i_fqdn)), '}}')::jsonb;
    ELSIF v_ctype = 'mysql_cluster' THEN
        RETURN concat('{"data": {"cluster_nodes":', (SELECT code.get_mysql_cluster_nodes(i_fqdn)), '}}')::jsonb;
    ELSE
        RETURN '{}'::jsonb;
    END IF;
END
$$ LANGUAGE plpgsql STABLE;
