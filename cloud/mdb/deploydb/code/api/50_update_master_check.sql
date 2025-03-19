CREATE OR REPLACE FUNCTION code.update_master_check(
    i_master_fqdn     text,
    i_checker_fqdn    text,
    i_is_alive        boolean
) RETURNS code.master AS $$
DECLARE
    v_master    deploy.masters;
    v_ts        timestamptz;
BEGIN
    v_ts := clock_timestamp();
    v_master := code._acquire_master(i_master_fqdn);

    -- Update checker view with new status
    INSERT INTO deploy.masters_check_view
      (master_id, checker_fqdn, is_alive, updated_at)
    VALUES
      ((v_master).master_id, i_checker_fqdn, i_is_alive, v_ts)
    ON CONFLICT (master_id, checker_fqdn)
        DO UPDATE SET
            is_alive = i_is_alive,
            updated_at = v_ts
        WHERE masters_check_view.master_id = (v_master).master_id
          AND masters_check_view.checker_fqdn = i_checker_fqdn;

    -- Calculate if master is really alive or dead (using quorum of checkers)
    v_master := code._update_master_alive(v_master, v_ts);

    RETURN code._as_master(
            v_master,
        (SELECT groups
           FROM deploy.groups
          WHERE group_id = (v_master).group_id),
        ARRAY(SELECT alias
           FROM deploy.master_aliases
          WHERE master_aliases.master_id = (v_master).master_id)
    );
END;
$$ LANGUAGE plpgsql;
