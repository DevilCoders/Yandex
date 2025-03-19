CREATE OR REPLACE FUNCTION code._update_master_alive(
    i_master    deploy.masters,
    i_ts        timestamptz
) RETURNS deploy.masters AS $$
DECLARE
    v_total         bigint;
    v_alive         bigint;
BEGIN
    SELECT count(*), count(CASE WHEN is_alive THEN 1 END) INTO v_total, v_alive
      FROM deploy.masters_check_view
    WHERE master_id = (i_master).master_id
      AND updated_at + INTERVAL '5 minutes' >= i_ts;

    -- Master is alive if we have n / 2 + 1 of 'alive' views
    IF v_total / 2 + 1 <=  v_alive THEN
        -- Update if necessary
        IF NOT (i_master).is_alive THEN
            UPDATE deploy.masters
              SET is_alive = true
            WHERE master_id = (i_master).master_id
              RETURNING * INTO i_master;
        END IF;

        RETURN i_master;
    END IF;

    -- Master is viewed as dead, update and failover minions
    UPDATE deploy.masters
      SET is_alive = false
    WHERE master_id = (i_master).master_id
      RETURNING * INTO i_master;

    RETURN i_master;
END;
$$ LANGUAGE plpgsql;
