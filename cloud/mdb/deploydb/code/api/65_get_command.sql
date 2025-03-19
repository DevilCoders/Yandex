CREATE OR REPLACE FUNCTION code.get_command(i_command_id bigint)
RETURNS SETOF code.command AS $$
SELECT (code._as_command(commands, shipment_commands, minions)).*
  FROM deploy.commands
  JOIN deploy.minions
 USING (minion_id)
  JOIN deploy.shipment_commands
 USING (shipment_command_id)
 WHERE commands.command_id = i_command_id;
$$ LANGUAGE SQL STABLE;
