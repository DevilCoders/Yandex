CREATE OR REPLACE FUNCTION code.upsert_master(
    i_fqdn         text,
    i_deploy_group text DEFAULT NULL,
    i_is_open      boolean DEFAULT NULL,
    i_description  text DEFAULT NULL
) RETURNS code.master AS $$
DECLARE
    v_group  deploy.groups;
    v_master deploy.masters;
BEGIN
    -- Check if we have old minion
    SELECT * INTO v_master
        FROM deploy.masters
    WHERE fqdn = i_fqdn
        FOR UPDATE;

    IF NOT found THEN
        v_group := code._get_group(i_deploy_group);

        -- Insert or update if someone inserted before us
        INSERT INTO deploy.masters
            (group_id, fqdn, is_open, description, is_alive)
        VALUES
            (v_group.group_id, i_fqdn, i_is_open, i_description, false)
        ON CONFLICT (fqdn)
            DO UPDATE SET
                group_id = v_group.group_id,
                is_open = i_is_open,
                description = i_description
            WHERE masters.fqdn = i_fqdn
        RETURNING * INTO v_master;
    ELSE
        -- We are updating old master
        IF i_deploy_group IS NOT NULL THEN
            -- We are updating group
            v_group := code._get_group(i_deploy_group);
        ELSE
            -- We are not updating group, get current group and master
            SELECT * INTO v_group
                FROM deploy.groups
            WHERE group_id = v_master.group_id;
        END IF;

        -- Update
        UPDATE deploy.masters
        SET
            group_id = v_group.group_id,
            is_open = CASE WHEN i_is_open IS NOT NULL THEN i_is_open ELSE masters.is_open END,
            description = CASE WHEN i_description IS NOT NULL THEN i_description ELSE masters.description END
        WHERE masters.fqdn = i_fqdn
        RETURNING * INTO v_master;
    END IF;

    PERFORM code._log_master_change(
        'create', i_new_row => v_master
    );

    RETURN code._as_master(v_master, v_group, '{}'::text[]);
END;
$$ LANGUAGE plpgsql;
