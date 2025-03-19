CREATE OR REPLACE FUNCTION code.get_commands_for_dispatch(
    i_fqdn             text,
    i_limit            bigint,
    i_throttling_count bigint DEFAULT 10
)
RETURNS SETOF code.command AS $$
WITH running_commands AS (
    SELECT i_throttling_count - COUNT(*) AS unthrottle_count
    FROM deploy.commands
    JOIN deploy.minions USING (minion_id)
    JOIN deploy.masters USING (master_id)
    WHERE status = 'RUNNING' AND masters.fqdn = i_fqdn
)
SELECT (code._as_command(commands, shipment_commands, minions)).*
  FROM deploy.commands
  JOIN deploy.minions
 USING (minion_id)
  JOIN deploy.masters
 USING (master_id)
  JOIN deploy.shipment_commands
 USING (shipment_command_id)
 WHERE masters.fqdn = i_fqdn
   AND masters.is_alive = true
   AND minions.deleted = false
   AND commands.status = 'AVAILABLE'
   AND (commands.last_dispatch_attempt_at IS NULL OR commands.last_dispatch_attempt_at + INTERVAL '15 seconds' <= now())
 ORDER BY commands.updated_at ASC
 LIMIT LEAST(i_limit, (SELECT GREATEST(0, unthrottle_count) FROM running_commands))
 FOR NO KEY UPDATE OF commands SKIP LOCKED;
$$ LANGUAGE SQL;
