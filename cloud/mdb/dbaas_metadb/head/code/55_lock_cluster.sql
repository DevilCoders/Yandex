CREATE OR REPLACE FUNCTION code.lock_cluster(
    i_cid          text,
    i_x_request_id text DEFAULT NULL
) RETURNS SETOF code.cluster_with_labels AS $$
DECLARE
  v_cluster dbaas.clusters;
  v_cluster_in_maintenance boolean DEFAULT false;
BEGIN
  SELECT actual_rev <> next_rev
  INTO v_cluster_in_maintenance
  FROM dbaas.clusters
  WHERE cid = i_cid;

  v_cluster := code.forward_cluster_revision(
      i_cid => i_cid,
      i_x_request_id => i_x_request_id);

  IF v_cluster_in_maintenance THEN
    PERFORM code.cancel_maintenance_tasks(
      i_cid => i_cid,
      i_rev => (v_cluster).actual_rev);
  END IF;

  RETURN QUERY
    SELECT cl.*
      FROM code.as_cluster_with_labels(v_cluster) cl;
END;
$$ LANGUAGE plpgsql;
