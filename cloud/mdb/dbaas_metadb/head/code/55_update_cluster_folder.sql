CREATE OR REPLACE FUNCTION code.update_cluster_folder(
    i_cid       text,
    i_folder_id bigint,
    i_rev       bigint
) RETURNS SETOF code.cluster_with_labels AS $$
DECLARE
    v_cluster dbaas.clusters;
BEGIN
    UPDATE dbaas.clusters
       SET folder_id = i_folder_id
     WHERE cid = i_cid
       AND folder_id != i_folder_id
       AND code.visible(clusters)
    RETURNING * INTO v_cluster;

    IF NOT found THEN
        RAISE EXCEPTION 'Cluster % not exists, invisible or already in folder %', i_cid, i_folder_id
            USING TABLE = 'dbaas.clusters';
    END IF;

    PERFORM code.update_cluster_change(
        i_cid,
        i_rev,
        jsonb_build_object(
            'update_cluster_folder',
            jsonb_build_object(
                'folder_id', i_folder_id
            )
        )
    );

    RETURN QUERY
        SELECT cl.*
          FROM code.as_cluster_with_labels(v_cluster) cl;
END;
$$ LANGUAGE plpgsql;
