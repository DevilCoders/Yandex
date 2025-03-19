CREATE OR REPLACE FUNCTION code._acquire_command(
    i_command_id bigint
) RETURNS deploy.commands AS $$
DECLARE
    v_command deploy.commands;
BEGIN
    SELECT * INTO v_command
        FROM deploy.commands
    WHERE command_id = i_command_id
        FOR UPDATE;

    IF NOT found THEN
        RAISE EXCEPTION 'Unable to find command %', i_command_id
                  USING ERRCODE = code._error_not_found(), TABLE = 'deploy.commands';
    END IF;

    RETURN v_command;
END;
$$ LANGUAGE plpgsql;
