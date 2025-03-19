CREATE OR REPLACE FUNCTION code.revert_cluster_to_rev(
    i_cid    text,
    i_rev    bigint,
    i_reason text
) RETURNS bigint AS $$
DECLARE
    v_locked_cluster   code.cluster_with_labels;
BEGIN
    v_locked_cluster := code.lock_cluster(
      i_cid          => i_cid,
      i_x_request_id => i_reason);

    PERFORM code.reset_cluster_to_rev(
        i_cid => i_cid,
        i_rev => i_rev
    );

    PERFORM code.update_cluster_change(
        i_cid,
        (v_locked_cluster).rev,
        jsonb_build_object(
            'revert_cluster_to_rev',
             jsonb_build_object(
                'i_rev', i_rev
            )
        )
    );

    PERFORM code.complete_cluster_change(i_cid, (v_locked_cluster).rev);

    RETURN (v_locked_cluster).rev;
END;
$$ LANGUAGE plpgsql;
