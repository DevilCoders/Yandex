CREATE OR REPLACE FUNCTION code.update_cluster_deletion_protection(
    i_cid                 text,
    i_deletion_protection bool,
    i_rev                 bigint
) RETURNS SETOF code.cluster_with_labels AS $$
DECLARE
    v_cluster dbaas.clusters;
BEGIN
    UPDATE dbaas.clusters
       SET deletion_protection = i_deletion_protection
     WHERE cid = i_cid
    RETURNING * INTO v_cluster;

    PERFORM code.update_cluster_change(
        i_cid,
        i_rev,
        jsonb_build_object(
            'update_cluster_deletion_protection',
            jsonb_build_object(
                'deletion_protection', i_deletion_protection
            )
        )
    );
    RETURN QUERY
        SELECT cl.*
          FROM code.as_cluster_with_labels(v_cluster) cl;
END;
$$ LANGUAGE plpgsql;
