CREATE OR REPLACE FUNCTION code.get_commands(
    i_shipment_id       bigint,
    i_fqdn              text,
    i_status            deploy.command_status,
    i_limit             bigint,
    i_last_command_id   bigint DEFAULT NULL,
    i_ascending         boolean DEFAULT true
)
RETURNS SETOF code.command AS $$
SELECT (code._as_command(commands, shipment_commands, minions)).*
  FROM deploy.commands
  JOIN deploy.minions
 USING (minion_id)
  JOIN deploy.shipment_commands
 USING (shipment_command_id)
 WHERE (i_shipment_id IS NULL OR shipment_id = i_shipment_id)
   AND (i_fqdn IS NULL OR fqdn = i_fqdn)
   AND (i_status IS NULL OR commands.status = i_status)
   AND (i_last_command_id IS NULL OR commands.command_id > i_last_command_id)
 ORDER BY commands.command_id * CASE WHEN i_ascending THEN 1 ELSE -1 END ASC
 LIMIT i_limit;
$$ LANGUAGE SQL STABLE;
