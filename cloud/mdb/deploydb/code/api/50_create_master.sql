CREATE OR REPLACE FUNCTION code.create_master(
    i_fqdn         text,
    i_deploy_group text,
    i_is_open      boolean,
    i_description  text DEFAULT NULL
) RETURNS code.master AS $$
DECLARE
    v_group  deploy.groups;
    v_master deploy.masters;
BEGIN
    v_group := code._get_group(i_deploy_group);

    INSERT INTO deploy.masters
        (group_id, fqdn, is_open, description, is_alive)
    VALUES
        (v_group.group_id, i_fqdn, i_is_open, i_description, false)
    RETURNING * INTO v_master;

    PERFORM code._log_master_change(
        'create', i_new_row => v_master
    );

    RETURN code._as_master(v_master, v_group, '{}'::text[]);
END;
$$ LANGUAGE plpgsql;