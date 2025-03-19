CREATE OR REPLACE FUNCTION code.upsert_minion(
  i_fqdn            text,
  i_deploy_group    text DEFAULT NULL,
  i_auto_reassign   boolean DEFAULT NULL,
  i_register_ttl    interval DEFAULT '1 day',
  i_allow_recreate  boolean DEFAULT false,
  i_master          text DEFAULT NULL
) RETURNS code.minion AS $$
DECLARE
  v_group  deploy.groups;
  v_master deploy.masters;
  v_minion deploy.minions;
BEGIN
  IF i_deploy_group IS NOT NULL AND i_master IS NOT NULL THEN
      RAISE EXCEPTION 'Unable to set both master and group.'
           USING ERRCODE = code._error_invalid_input();
  END IF;

  -- Check if we have old minion
  SELECT * INTO v_minion
    FROM deploy.minions
  WHERE fqdn = i_fqdn
    FOR UPDATE;

  IF NOT found THEN
    -- We are creating new minion
    IF i_master IS NOT NULL THEN
        SELECT * INTO v_master
        FROM deploy.masters
        WHERE fqdn = i_master;
        IF NOT found THEN
            RAISE EXCEPTION 'Unable to find master with name %:', i_master
                USING ERRCODE = code._error_not_found();
        END IF;

        SELECT * INTO v_group
        FROM deploy.groups
        WHERE group_id = v_master.group_id;
        IF NOT found THEN
            RAISE EXCEPTION 'Unable to find group for master %:', v_master.fqdn
                USING ERRCODE = code._error_not_found();
        END IF;
    ELSE
        v_group := code._get_group(i_deploy_group);
        v_master := code._find_open_master(v_group);
    END IF;
    -- Insert or update if someone inserted before us
    INSERT INTO deploy.minions
      (fqdn, group_id, master_id, auto_reassign, register_until)
    VALUES
      (i_fqdn, v_group.group_id, v_master.master_id, i_auto_reassign, now() + i_register_ttl)
    ON CONFLICT (fqdn)
      DO UPDATE
      SET
        group_id = v_group.group_id,
        master_id = v_master.master_id,
        auto_reassign = i_auto_reassign
      WHERE minions.fqdn = i_fqdn
    RETURNING * INTO v_minion;
  ELSE
    -- We are updating old minion
    IF i_deploy_group IS NOT NULL THEN
      -- We are updating group
      v_group := code._get_group(i_deploy_group);
      IF v_group.group_id != v_minion.group_id THEN
        -- Group is different - find new open master in that group
        v_master := code._find_open_master(v_group);
      ELSE
        SELECT * INTO v_master
          FROM deploy.masters
        WHERE master_id = v_minion.master_id;
      END IF;
    ELSIF i_master IS NOT NULL THEN
        SELECT * INTO v_master
        FROM deploy.masters
        WHERE fqdn = i_master;
        IF NOT found THEN
            RAISE EXCEPTION 'Unable to find master with name %:', i_master
                USING ERRCODE = code._error_not_found();
        END IF;

        SELECT * INTO v_group
        FROM deploy.groups
        WHERE group_id = v_master.group_id;
        IF NOT found THEN
            RAISE EXCEPTION 'Unable to find group for master %:', v_master.fqdn
                USING ERRCODE = code._error_not_found();
        END IF;
    ELSE
      -- We are not updating group, get current group and master
      SELECT * INTO v_group
        FROM deploy.groups
      WHERE group_id = v_minion.group_id;
      SELECT * INTO v_master
        FROM deploy.masters
      WHERE master_id = v_minion.master_id;
    END IF;

    -- Update
    IF i_allow_recreate AND v_minion.deleted THEN
      UPDATE deploy.minions
      SET group_id       = v_group.group_id,
          master_id      = v_master.master_id,
          auto_reassign  = CASE WHEN i_auto_reassign IS NOT NULL THEN i_auto_reassign ELSE minions.auto_reassign END,
          pub_key        = NULL,
          register_until = now() + i_register_ttl,
          deleted        = false
      WHERE minions.fqdn = i_fqdn
      RETURNING * INTO v_minion;
    ELSE
      UPDATE deploy.minions
      SET group_id       = v_group.group_id,
          master_id      = v_master.master_id,
          auto_reassign  = CASE WHEN i_auto_reassign IS NOT NULL THEN i_auto_reassign ELSE minions.auto_reassign END
      WHERE minions.fqdn = i_fqdn
      RETURNING * INTO v_minion;
    END IF;
  END IF;

  PERFORM code._log_minion_change(
    'create', i_new_row => v_minion
  );

  RETURN code._as_minion(v_minion, v_master, v_group);
END;
$$ LANGUAGE plpgsql;
