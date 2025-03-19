CREATE OR REPLACE FUNCTION code.create_minion(
    i_fqdn          text,
    i_deploy_group  text,
    i_auto_reassign boolean,
    i_register_ttl  interval DEFAULT '1 day'
) RETURNS code.minion AS $$
DECLARE
    v_group  deploy.groups;
    v_master deploy.masters;
    v_minion deploy.minions;
BEGIN
    v_group := code._get_group(i_deploy_group);
    v_master := code._find_open_master(v_group);

    INSERT INTO deploy.minions
        (fqdn, group_id, master_id, auto_reassign, register_until)
    VALUES
        (i_fqdn, v_group.group_id, v_master.master_id, i_auto_reassign, now() + i_register_ttl)
    RETURNING * INTO v_minion;

    PERFORM code._log_minion_change(
        'create', i_new_row => v_minion
    );

    RETURN code._as_minion(v_minion, v_master, v_group);
END;
$$ LANGUAGE plpgsql;