CREATE OR REPLACE FUNCTION code.failover_minions(
    i_limit     bigint
) RETURNS SETOF code.minion AS $$
DECLARE
    v_count         bigint;
    v_group         deploy.groups;
    v_new_master    deploy.masters;
    v_minion        deploy.minions;
    v_new_minion    deploy.minions;
    v_ts            timestamptz;
BEGIN
    v_ts := clock_timestamp();

    FOR v_minion IN
        SELECT minions.*
          FROM deploy.minions
          JOIN deploy.masters
        USING (master_id)
        WHERE masters.is_alive = false
          AND minions.auto_reassign = true
        FOR UPDATE SKIP LOCKED
    LOOP
        SELECT * INTO v_group
          FROM deploy.groups
        WHERE group_id = (v_minion).group_id;

        BEGIN
            v_new_master := code._find_open_master(v_group);
        EXCEPTION WHEN OTHERS THEN
            RAISE LOG 'Failed to find open master for minion % in group %', (v_minion).minion_id, (v_group).group_id;
            -- We skip to next minion without incrementing counter or we might starve ourselves
            CONTINUE;
        END;

        UPDATE deploy.minions
        SET master_id = (v_new_master).master_id,
            updated_at = v_ts
        WHERE minion_id = (v_minion).minion_id
            RETURNING * INTO v_new_minion;

        PERFORM code._log_minion_change(
            'reassign',
            i_old_row => v_minion,
            i_new_row => v_new_minion
        );

        RETURN NEXT code._as_minion(v_new_minion, v_new_master, v_group);

        v_count := v_count + 1;
        EXIT WHEN v_count >= i_limit;
    END LOOP;
END;
$$ LANGUAGE plpgsql;
