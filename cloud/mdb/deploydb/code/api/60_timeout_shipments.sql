CREATE OR REPLACE FUNCTION code.timeout_shipments(
    i_limit bigint
) RETURNS SETOF code.shipment AS $$
DECLARE
    v_shipments             deploy.shipments;
    v_shipment_commands     deploy.shipment_commands;
    v_shipment_commands_arr deploy.shipment_commands[];
    v_command_defs          code.command_def[];
    v_ts                    timestamptz;
BEGIN
    v_ts := clock_timestamp();

    FOR v_shipments IN
        SELECT shipments.*
          FROM deploy.shipments
        WHERE status = 'INPROGRESS'
          AND created_at + timeout <= v_ts
        LIMIT i_limit
        FOR UPDATE SKIP LOCKED
    LOOP
        UPDATE deploy.shipments
          SET status = 'TIMEOUT',
              updated_at = v_ts
        WHERE shipment_id = (v_shipments).shipment_id
        RETURNING * INTO v_shipments;

        SELECT array_agg(shipment_commands.* ORDER BY shipment_command_id ASC)
          INTO v_shipment_commands_arr
          FROM deploy.shipment_commands
        WHERE shipment_id = (v_shipments).shipment_id;

        FOREACH v_shipment_commands IN ARRAY v_shipment_commands_arr
        LOOP
            UPDATE deploy.commands
            SET status = 'CANCELED',
                updated_at = v_ts
            WHERE commands.shipment_command_id = v_shipment_commands.shipment_command_id
              AND status IN ('AVAILABLE', 'BLOCKED');

            v_command_defs := array_append(v_command_defs, code._as_command_def(v_shipment_commands));
        END LOOP;

        RETURN NEXT code._as_shipment(v_shipments, v_command_defs);
    END LOOP;
END;
$$ LANGUAGE plpgsql;
