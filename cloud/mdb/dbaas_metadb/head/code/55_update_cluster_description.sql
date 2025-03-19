CREATE OR REPLACE FUNCTION code.update_cluster_description(
    i_cid         text,
    i_description text,
    i_rev         bigint
) RETURNS SETOF code.cluster_with_labels AS $$
DECLARE
    v_cluster dbaas.clusters;
BEGIN
    UPDATE dbaas.clusters
       SET description = i_description
     WHERE cid = i_cid
    RETURNING * INTO v_cluster;

    PERFORM code.update_cluster_change(
        i_cid,
        i_rev,
        jsonb_build_object(
            'update_cluster_description',
            jsonb_build_object(
                'description', i_description
            )
        )
    );
    RETURN QUERY
        SELECT cl.*
          FROM code.as_cluster_with_labels(v_cluster) cl;
END;
$$ LANGUAGE plpgsql;
