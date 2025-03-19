CREATE OR REPLACE FUNCTION code.lock_future_cluster(
    i_cid          text,
    i_x_request_id text DEFAULT NULL
) RETURNS SETOF code.cluster_with_labels AS $$
DECLARE
    v_cluster dbaas.clusters;
BEGIN
    UPDATE dbaas.clusters
    SET next_rev = next_rev + 1
    WHERE cid = i_cid
    RETURNING * INTO v_cluster;

    IF NOT found THEN
        RAISE EXCEPTION 'There is no cluster %', i_cid
            USING TABLE = 'dbaas.clusters';
    END IF;

    INSERT INTO dbaas.clusters_changes
    (cid, rev, x_request_id)
    VALUES
    (i_cid, (v_cluster).next_rev, i_x_request_id);

    RETURN QUERY
        SELECT cl.*
        FROM code.as_cluster_with_labels(v_cluster) cl;
END;
$$ LANGUAGE plpgsql;
