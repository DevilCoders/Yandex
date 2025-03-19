CREATE OR REPLACE FUNCTION code.forward_cluster_revision(
    i_cid          text,
    i_x_request_id text DEFAULT NULL
) RETURNS dbaas.clusters AS $$
DECLARE
    v_cluster dbaas.clusters;
BEGIN
    UPDATE dbaas.clusters
    SET actual_rev = next_rev + 1,
        next_rev = next_rev + 1
    WHERE cid = i_cid
    RETURNING * INTO v_cluster;

    IF NOT found THEN
      RAISE EXCEPTION 'Unable to find cluster cid=% to lock', i_cid
        USING TABLE = 'dbaas.clusters';
    END IF;

    INSERT INTO dbaas.clusters_changes
      (cid, rev, x_request_id)
    VALUES
      (i_cid, (v_cluster).actual_rev, i_x_request_id);

    RETURN v_cluster;
END;
$$ LANGUAGE plpgsql;
