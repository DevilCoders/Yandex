CREATE OR REPLACE FUNCTION code.delete_hosts(
    i_fqdns text[],
    i_cid   text,
    i_rev   bigint
) RETURNS SETOF code.host AS $$
DECLARE
    v_hosts dbaas.hosts[];
BEGIN
    DELETE FROM dbaas.pillar
     WHERE fqdn = ANY(i_fqdns);

    WITH deleted AS (
        DELETE FROM dbaas.hosts
         WHERE fqdn = ANY(i_fqdns)
        RETURNING *
    )
    SELECT array_agg(dh::dbaas.hosts)
      INTO v_hosts
      FROM deleted dh;

    PERFORM code.update_cluster_change(
        i_cid,
        i_rev,
        jsonb_build_object(
            'delete_hosts',
            jsonb_build_object(
                'fqdns', i_fqdns
            )
        )
    );

    RETURN QUERY
        SELECT h.*
          FROM unnest(v_hosts) dh,
               code.format_host(dh) h;
END;
$$ LANGUAGE plpgsql;
