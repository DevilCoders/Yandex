CREATE OR REPLACE FUNCTION code._update_command_status(
    i_command  deploy.commands,
    job_status deploy.job_status,
    ts timestamptz
) RETURNS void AS $$
DECLARE
    v_status           deploy.command_status;
    v_script           code.shipment_script;
    v_shipment_status  deploy.shipment_status;
BEGIN
    IF (i_command).status != 'RUNNING' THEN
        RETURN;
    END IF;

    v_status := CASE
        WHEN job_status = 'DONE' THEN 'DONE'::deploy.command_status
        WHEN job_status = 'ERROR' THEN 'ERROR'::deploy.command_status
        WHEN job_status = 'TIMEOUT' THEN 'TIMEOUT'::deploy.command_status
    END;

    UPDATE deploy.commands
       SET status = v_status,
           updated_at = ts
     WHERE command_id = (i_command).command_id;

    v_script := code._get_shipment_script((i_command).shipment_command_id);

    IF v_status IN  ('ERROR', 'TIMEOUT') THEN
        -- Update shipment, cause we finished that script
        v_shipment_status := code._update_shipment_status(
            i_shipment_id  => (v_script).shipment_id,
            i_errors_delta => 1,
            i_done_delta   => 0,
            i_other_delta  => -1,
            i_ts           => ts
        );
    ELSE
        IF (v_script).next_shipment_command_id IS NOT NULL THEN
            -- We might not update anything here at all because command might be CANCELED already
            UPDATE deploy.commands
               SET status = 'AVAILABLE',
                   updated_at = ts
             WHERE minion_id = (i_command).minion_id
               AND shipment_command_id = (v_script).next_shipment_command_id
               AND status = 'BLOCKED'::deploy.command_status;

            -- Exit, cause we didn't finish that script yet,
            -- and we don't need to update counters and trigger new commands
            RETURN;
        END IF;
        -- No more commands in that script
        -- Update shipment, cause we finished that script
        v_shipment_status := code._update_shipment_status(
            i_shipment_id  => (v_script).shipment_id,
            i_errors_delta => 0,
            i_done_delta   => 1,
            i_other_delta  => -1,
            i_ts           => ts
        );
    END IF;

    CASE v_shipment_status
        WHEN 'DONE' THEN
            NULL;
        WHEN 'INPROGRESS' THEN
            UPDATE deploy.commands
               SET status = 'AVAILABLE',
                   updated_at = ts
             WHERE command_id IN (
                SELECT command_id
                  FROM deploy.commands
                 WHERE shipment_command_id = (v_script).first_shipment_command_id
                   AND status = 'BLOCKED'
                 ORDER BY command_id
                 LIMIT 1
            );
        WHEN 'ERROR' THEN
            UPDATE deploy.commands
               SET status = 'CANCELED',
                   updated_at = ts
             WHERE shipment_command_id = ANY((v_script).shipment_command_ids)
               AND status IN ('AVAILABLE', 'BLOCKED');
        ELSE
            RAISE EXCEPTION 'Got unexpected v_shipment_status "%"', v_shipment_status;
    END CASE;
END;
$$ LANGUAGE plpgsql;
