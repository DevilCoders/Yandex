CREATE OR REPLACE FUNCTION v1_code.register_lifecycle_scheduling(
    i_key bigint,
    i_scheduling_ts timestamptz
) RETURNS v1_code.lifecycle_scheduling_status LANGUAGE plpgsql AS
$$
DECLARE
    result v1_code.lifecycle_scheduling_status;
BEGIN
    INSERT INTO s3.lifecycle_scheduling_status (key, scheduling_ts)
        VALUES (i_key, i_scheduling_ts) ON CONFLICT DO NOTHING
        RETURNING key, scheduling_ts, last_processed_bucket, status INTO result;

    IF result.key IS NOT NULL THEN
        RETURN result;
    END IF;

    SELECT key, scheduling_ts, last_processed_bucket, status
        INTO result
        FROM s3.lifecycle_scheduling_status
        WHERE key = i_key;
    return result;
END;
$$;
