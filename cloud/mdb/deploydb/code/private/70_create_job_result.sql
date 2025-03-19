CREATE OR REPLACE FUNCTION code._create_job_result(
    i_ext_job_id    text,
    i_fqdn          text,
    i_status        deploy.job_result_status,
    i_result        jsonb,
    i_ts            timestamptz
) RETURNS code.job_result AS $$
DECLARE
    v_job deploy.jobs;
    v_job_result deploy.job_results;
    v_inserted   bigint;
    v_order_id   integer;
BEGIN
    v_order_id := 1;

    -- Try to insert as first result
    INSERT INTO deploy.job_results AS jr
    (ext_job_id, fqdn, order_id, status, result, recorded_at)
    VALUES
    (i_ext_job_id, i_fqdn, v_order_id, i_status, i_result, i_ts)
    ON CONFLICT (ext_job_id, fqdn, order_id)
        DO UPDATE SET
                      status = i_status,
                      result = i_result,
                      recorded_at = i_ts
        WHERE jr.status IS NULL
          RETURNING * INTO v_job_result;

    LOOP
        GET DIAGNOSTICS v_inserted = ROW_COUNT;

        -- Check if we managed to insert anything
        IF v_inserted != 0 THEN
            -- We did! Job is done
            EXIT;
        END IF;

        -- Get current count of inserted results for this job id and fqdn
        SELECT COUNT(*) + 1 INTO v_order_id
        FROM deploy.job_results
        WHERE ext_job_id = i_ext_job_id AND fqdn = i_fqdn;

        -- Try inserting new result with 'next' id
        INSERT INTO deploy.job_results
        (ext_job_id, fqdn, order_id, status, result, recorded_at)
        VALUES
        (i_ext_job_id, i_fqdn, v_order_id, i_status, i_result, i_ts)
        ON CONFLICT DO NOTHING
            RETURNING * INTO v_job_result;
        -- Repeat the loop...
    END LOOP;

    SELECT * INTO v_job FROM code._get_job(i_ext_job_id, i_fqdn);

    -- If job is found AND inserted id of job result is 1, update its status
    IF v_job IS NOT NULL AND v_order_id = 1 THEN
        v_job := code._update_job_status(v_job, v_job_result, i_ts);
    END IF;

    RETURN code._as_job_result(v_job_result);
END;
$$ LANGUAGE plpgsql;
