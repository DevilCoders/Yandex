CREATE OR REPLACE FUNCTION code.lock_cluster_by_task(
    i_task_id text
) RETURNS bigint AS $$
DECLARE
    v_cid         text;
    v_target_rev  bigint;
    v_actual_rev  bigint;
    v_acquire_rev bigint;
BEGIN
    SELECT c.cid, wq.target_rev, c.actual_rev, wq.acquire_rev
    INTO v_cid, v_target_rev, v_actual_rev, v_acquire_rev
    FROM dbaas.worker_queue wq
             JOIN dbaas.clusters c USING (cid)
    WHERE task_id = i_task_id;

    IF NOT found THEN
        RAISE EXCEPTION 'Unable to lock cluster by task %. No task with that task_id found', i_task_id
              USING TABLE = 'dbaas.worker_queue';
    END IF;

    IF v_target_rev IS NOT NULL AND v_acquire_rev IS NULL THEN
        IF v_actual_rev >= v_target_rev THEN
            RAISE EXCEPTION 'In task % target rev need to be future revision, actual %, target %', i_task_id, v_actual_rev, v_target_rev
                USING TABLE = 'dbaas.worker_queue';
        END IF;
        PERFORM code.reset_cluster_to_rev(v_cid, v_target_rev);
        RETURN (code.forward_cluster_revision(v_cid)).actual_rev;
    END IF;

    RETURN (code.lock_cluster(v_cid)).rev;
END;
$$ LANGUAGE plpgsql;
