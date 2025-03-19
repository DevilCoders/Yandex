CREATE OR REPLACE FUNCTION code.complete_future_cluster_change(
    i_cid  text,
    i_actual_rev  bigint,
    i_next_rev bigint
) RETURNS void AS $$
BEGIN
    IF i_actual_rev = i_next_rev THEN
        RAISE EXCEPTION 'Unable to complete future cluster change: '
                        'actual and next rev are equals cid=%, rev=%', i_cid, i_actual_rev
            USING TABLE = 'dbaas.clusters';
    END IF;
    PERFORM code.complete_cluster_change(i_cid, i_next_rev);
    PERFORM code.reset_cluster_to_rev(i_cid, i_actual_rev);
END;
$$ LANGUAGE plpgsql;
