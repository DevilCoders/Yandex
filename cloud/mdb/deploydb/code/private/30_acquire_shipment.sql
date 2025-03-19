CREATE OR REPLACE FUNCTION code._acquire_shipment(
    i_shipment_command_id bigint
) RETURNS deploy.shipments AS $$
DECLARE
    v_shipment deploy.shipments;
BEGIN
    SELECT * INTO v_shipment
      FROM deploy.shipments
     WHERE shipment_id = (
        SELECT shipment_id
          FROM deploy.shipment_commands
         WHERE shipment_command_id = i_shipment_command_id)
      FOR UPDATE;

    IF NOT found THEN
        RAISE EXCEPTION 'Unable to find shipment by shipment_command_id: %', i_shipment_command_id
                  USING ERRCODE = code._error_not_found(), TABLE = 'deploy.shipments';
    END IF;

    RETURN v_shipment;
END;
$$ LANGUAGE plpgsql;
