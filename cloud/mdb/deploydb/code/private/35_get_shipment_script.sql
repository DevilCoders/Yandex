CREATE OR REPLACE FUNCTION code._get_shipment_script(
    i_shipment_command_id bigint
) RETURNS code.shipment_script AS $$
SELECT (shipment_id,
        min(shipment_command_id),
        min(shipment_command_id) FILTER (WHERE shipment_command_id > i_shipment_command_id),
        array_agg(shipment_command_id))::code.shipment_script
  FROM deploy.shipment_commands
 WHERE shipment_id = (
     SELECT shipment_id
       FROM deploy.shipment_commands
      WHERE shipment_command_id = i_shipment_command_id)
 GROUP BY shipment_id;
$$ LANGUAGE SQL STABLE;
