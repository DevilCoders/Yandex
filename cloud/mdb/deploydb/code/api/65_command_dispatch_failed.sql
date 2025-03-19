CREATE OR REPLACE FUNCTION code.command_dispatch_failed(
    i_command_id    bigint
) RETURNS code.command AS $$
DECLARE
    v_command deploy.commands;
    v_ts timestamptz;
BEGIN
    v_ts := clock_timestamp();

    UPDATE deploy.commands
      SET updated_at = v_ts,
          last_dispatch_attempt_at = v_ts
    WHERE command_id = i_command_id
    RETURNING * INTO v_command;

    RETURN code._as_command(
        v_command,
        (SELECT shipment_commands FROM deploy.shipment_commands WHERE shipment_command_id = (v_command).shipment_command_id),
        (SELECT minions FROM deploy.minions WHERE minions.minion_id = (v_command).minion_id));
END;
$$ LANGUAGE plpgsql;
