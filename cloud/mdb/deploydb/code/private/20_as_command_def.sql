CREATE OR REPLACE FUNCTION code._as_command_def(
    deploy.shipment_commands
) RETURNS code.command_def AS $$
SELECT $1.type, $1.arguments, $1.timeout;
$$ LANGUAGE SQL IMMUTABLE;

CREATE OR REPLACE FUNCTION code._as_command_def(
    deploy.shipment_commands[]
) RETURNS code.command_def[] AS $$
SELECT array_agg(code._as_command_def(cd_row))
  FROM unnest($1) cd_row
$$ LANGUAGE SQL IMMUTABLE;
