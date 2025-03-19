CREATE OR REPLACE FUNCTION code.register_minion(
    i_fqdn     text,
    i_pub_key  text
) RETURNS code.minion AS $$
DECLARE
    v_minion deploy.minions;
    v_old_minion deploy.minions;
BEGIN
    v_old_minion := code._acquire_minion(i_fqdn);

    -- Deny if timeout passed
    IF v_old_minion.register_until IS NULL THEN
        RAISE EXCEPTION 'Unable to register minion %: already registered and not allowed to reregister',
            i_fqdn
        USING ERRCODE = code._error_already_registered();
    END IF;

    -- Deny if timeout passed
    IF v_old_minion.register_until < now() THEN
        RAISE EXCEPTION 'Unable to register minion %: registration timeout %',
            i_fqdn, (v_old_minion).register_until
        USING ERRCODE = code._error_registration_timeout();
    END IF;

    -- If its the same key, do not allow it.
    -- If its a different key, allow changing it because it might be reregistration.
    IF v_old_minion.pub_key IS NOT NULL THEN
        IF v_old_minion.pub_key = i_pub_key THEN
            -- Same key
            RAISE EXCEPTION 'Unable to register minion %: already registered with same key',
                i_fqdn
            USING ERRCODE = code._error_already_registered();
        END IF;
    END IF;

    -- Register
    UPDATE deploy.minions
       SET pub_key = i_pub_key,
           updated_at = now(),
           register_until = NULL
     WHERE minion_id = (v_old_minion).minion_id
     RETURNING * INTO v_minion;

    PERFORM code._log_minion_change(
        'register',
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
