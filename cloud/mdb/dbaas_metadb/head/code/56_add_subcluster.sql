CREATE OR REPLACE FUNCTION code.add_subcluster(
    i_cid         text,
    i_subcid      text,
    i_name        text,
    i_roles       dbaas.role_type[],
    i_rev         bigint
) RETURNS TABLE (cid text, subcid text, name text, roles dbaas.role_type[]) AS $$
BEGIN
    INSERT INTO dbaas.subclusters (
        cid, subcid,
        name, roles
    ) VALUES (
        i_cid, i_subcid,
        i_name, i_roles
    );

    PERFORM code.update_cluster_change(
        i_cid,
        i_rev,
        jsonb_build_object(
            'add_subcluster',
             jsonb_build_object(
                'subcid', i_subcid,
                'name', i_name,
                'roles', i_roles::text[]
            )
        )
    );

    RETURN QUERY SELECT i_cid, i_subcid, i_name, i_roles;
END;
$$ LANGUAGE plpgsql;
