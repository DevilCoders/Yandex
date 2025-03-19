CREATE OR REPLACE FUNCTION code.update_cluster_name(
    i_cid   text,
    i_name  text,
    i_rev   bigint
) RETURNS SETOF code.cluster_with_labels AS $$
DECLARE
    v_cluster dbaas.clusters;
BEGIN
    UPDATE dbaas.clusters
       SET name = i_name
     WHERE cid = i_cid
    RETURNING * INTO v_cluster;

    PERFORM code.update_cluster_change(
        i_cid,
        i_rev,
        jsonb_build_object(
            'update_cluster_name',
            jsonb_build_object(
                'name', i_name
            )
        )
    );

    RETURN QUERY
        SELECT cl.*
          FROM code.as_cluster_with_labels(v_cluster) cl;
END;
$$ LANGUAGE plpgsql;
