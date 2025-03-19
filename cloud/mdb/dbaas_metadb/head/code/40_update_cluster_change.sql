CREATE OR REPLACE FUNCTION code.update_cluster_change(
    i_cid     text,
    i_rev     bigint,
    i_change  jsonb
) RETURNS void AS $$
DECLARE
    v_change_xid bigint;
BEGIN
    UPDATE dbaas.clusters_changes
       SET changes = changes || jsonb_build_array(i_change)
     WHERE cid = i_cid
       AND rev = i_rev
    RETURNING commit_id INTO v_change_xid;

    IF NOT found THEN
        RAISE EXCEPTION 'Unable to update cluster change cid=%, rev=%', i_cid, i_rev;
    END IF;

    IF v_change_xid IS DISTINCT FROM txid_current() THEN
        RAISE EXCEPTION 'Attempt to modify cluster in different transaction. Change xid: %. Current xid: %.', v_change_xid, txid_current()
            USING HINT = 'Wrap your call in code.lock_cluster() ... code.complete_cluster_change() block';
    END IF;
END;
$$ LANGUAGE plpgsql;
