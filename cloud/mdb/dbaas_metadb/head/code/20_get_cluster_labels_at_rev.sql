CREATE OR REPLACE FUNCTION code.get_cluster_labels_at_rev(
    i_cid text,
    i_rev bigint
) RETURNS code.label[] AS $$
SELECT ARRAY(
    SELECT (label_key, label_value)::code.label
      FROM dbaas.cluster_labels_revs
      WHERE cid = i_cid
        AND rev = i_rev
    );
$$ LANGUAGE SQL STABLE;
