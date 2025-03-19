CREATE OR REPLACE FUNCTION code.get_rev_custom_pillar_by_host(
    i_fqdn         text,
    i_cluster_type dbaas.cluster_type,
    i_rev          bigint
) RETURNS jsonb AS $$
DECLARE
BEGIN
    IF i_cluster_type = 'postgresql_cluster' THEN
        RETURN concat('{"data": {"cluster_nodes":', (SELECT code.get_rev_pg_cluster_nodes(i_fqdn, i_rev)), '}}')::jsonb;
    ELSIF i_cluster_type = 'mysql_cluster' THEN
        RETURN concat('{"data": {"cluster_nodes":', (SELECT code.get_rev_mysql_cluster_nodes(i_fqdn, i_rev)), '}}')::jsonb;
    ELSE
        RETURN '{}'::jsonb;
    END IF;
END
$$ LANGUAGE plpgsql STABLE;
