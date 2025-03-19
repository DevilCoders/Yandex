CREATE OR REPLACE FUNCTION code.get_shipment(i_shipment_id bigint)
RETURNS SETOF code.shipment AS $$
SELECT (code._as_shipment(
    s, (SELECT array_agg(code._as_command_def(sc) ORDER BY shipment_command_id ASC)
          FROM deploy.shipment_commands sc
         WHERE sc.shipment_id = s.shipment_id))).*
  FROM deploy.shipments s
 WHERE s.shipment_id = i_shipment_id;
$$ LANGUAGE SQL STABLE;
