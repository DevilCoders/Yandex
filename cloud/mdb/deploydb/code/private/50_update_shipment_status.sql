CREATE OR REPLACE FUNCTION code._update_shipment_status(
    i_shipment_id  bigint,
    i_errors_delta int,
    i_done_delta   int,
    i_other_delta  int,
    i_ts           timestamptz
) RETURNS deploy.shipment_status AS $$
DECLARE
    v_shipment_status deploy.shipment_status;
BEGIN
    UPDATE deploy.shipments
       SET updated_at = i_ts,
           other_count = new_other_count,
           errors_count = new_errors_count,
           done_count = new_done_count,
           status = CASE
                WHEN NULLIF(new_errors_count, 0) >= NULLIF(stop_on_error_count, 0)
                    THEN 'ERROR'::deploy.shipment_status
                WHEN new_errors_count + new_done_count = total_count
                    THEN 'DONE'::deploy.shipment_status
                ELSE 'INPROGRESS'
           END
     FROM (
         SELECT other_count + i_other_delta AS new_other_count,
                errors_count + i_errors_delta AS new_errors_count,
                done_count + i_done_delta AS new_done_count
           FROM deploy.shipments
          WHERE shipment_id = i_shipment_id) new_counters
    WHERE shipment_id = i_shipment_id
    RETURNING status INTO v_shipment_status;

    IF NOT found THEN
        RAISE EXCEPTION 'Unable to find shipment by shipment_id: %', i_shipment_id
                  USING ERRCODE = code._error_not_found(), TABLE = 'deploy.shipments';
    END IF;

    RETURN v_shipment_status;
END;
$$ LANGUAGE plpgsql;
