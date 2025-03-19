CREATE OR REPLACE FUNCTION code.get_shipments(
    i_fqdn              text,
    i_status            deploy.shipment_status,
    i_limit             bigint,
    i_last_shipment_id  bigint DEFAULT NULL,
    i_ascending         boolean DEFAULT true
)
RETURNS SETOF code.shipment AS $$
SELECT (code._as_shipment(
    s, (SELECT array_agg(code._as_command_def(sc) ORDER BY shipment_command_id ASC)
          FROM deploy.shipment_commands sc
         WHERE sc.shipment_id = s.shipment_id))).*
  FROM deploy.shipments s
 WHERE (i_fqdn IS NULL OR i_fqdn = ANY(s.fqdns))
   AND (i_status IS NULL OR s.status = i_status)
   AND (i_last_shipment_id IS NULL
     OR CASE WHEN i_ascending THEN s.shipment_id > i_last_shipment_id ELSE s.shipment_id < i_last_shipment_id END)
 ORDER BY s.shipment_id * CASE WHEN i_ascending THEN 1 ELSE -1 END ASC
 LIMIT i_limit;
$$ LANGUAGE SQL STABLE;
