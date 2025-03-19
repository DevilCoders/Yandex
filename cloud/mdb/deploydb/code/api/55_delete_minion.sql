CREATE OR REPLACE FUNCTION code.delete_minion(
    i_fqdn text
) RETURNS SETOF code.minion AS $$
DECLARE
    v_minion deploy.minions;
BEGIN
    UPDATE deploy.minions
      SET deleted = true,
          updated_at = now()
    WHERE fqdn = i_fqdn AND deleted = false
      RETURNING * into v_minion;

    IF NOT found THEN
      RETURN;
    END IF;

    PERFORM code._log_minion_change(
        'delete', i_old_row => v_minion
    );

    RETURN NEXT code._as_minion(
        v_minion,
      (SELECT masters
         FROM deploy.masters
         WHERE master_id = (v_minion).master_id),
      (SELECT groups
         FROM deploy.groups
         WHERE group_id = (v_minion).group_id)
    );
END;
$$ LANGUAGE plpgsql;
