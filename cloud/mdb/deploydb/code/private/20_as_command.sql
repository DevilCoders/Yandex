CREATE OR REPLACE FUNCTION code._as_command(
    command   deploy.commands,
    shipment_command deploy.shipment_commands,
    minion    deploy.minions
) RETURNS code.command AS $$
SELECT $1.command_id,
       $2.shipment_id,
       $2.type,
       $2.arguments,
       minion.fqdn,
       $1.status::text,
       $1.created_at,
       $1.updated_at,
       $2.timeout;
$$ LANGUAGE SQL IMMUTABLE;
