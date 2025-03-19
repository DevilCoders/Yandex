CREATE OR REPLACE FUNCTION code.get_cluster_labels(
    i_cid text
) RETURNS code.label[] AS $$
SELECT ARRAY(
    SELECT (label_key, label_value)::code.label
      FROM dbaas.cluster_labels
      WHERE cid = i_cid
    );
$$ LANGUAGE SQL STABLE;
