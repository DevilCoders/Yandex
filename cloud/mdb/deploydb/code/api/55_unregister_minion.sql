CREATE OR REPLACE FUNCTION code.unregister_minion(
    i_fqdn          text,
    i_register_ttl  interval DEFAULT '1 day'
) RETURNS code.minion AS $$
DECLARE
    v_minion deploy.minions;
    v_old_minion deploy.minions;
BEGIN
    v_old_minion := code._acquire_minion(i_fqdn);

    -- We keep old key so that minion can change it gracefully
    UPDATE deploy.minions
       SET updated_at = now(),
           register_until = now() + i_register_ttl,
           deleted = false
     WHERE minion_id = (v_old_minion).minion_id
     RETURNING * INTO v_minion;

    PERFORM code._log_minion_change(
        'unregister',
        i_old_row => v_old_minion,
        i_new_row => v_minion
    );

    RETURN code._as_minion(
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
