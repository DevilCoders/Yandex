CREATE OR REPLACE FUNCTION code.set_labels_on_cluster(
    i_folder_id  bigint,
    i_cid        text,
    i_labels     code.label[],
    i_rev        bigint
) RETURNS void AS $$
BEGIN
    DELETE FROM dbaas.cluster_labels
     WHERE cid = i_cid;

    INSERT INTO dbaas.cluster_labels
        (cid, label_key, label_value)
    SELECT i_cid, key, value
      FROM unnest(i_labels);

    PERFORM code.update_cluster_change(
        i_cid,
        i_rev,
        jsonb_build_object(
            'set_labels_on_cluster',
            jsonb_build_object(
                'labels', i_labels
            )
        )
    );
END;
$$ LANGUAGE plpgsql;
