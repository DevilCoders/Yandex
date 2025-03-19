CREATE OR REPLACE FUNCTION v1_code.update_lifecycle_scheduling_status(
    i_key bigint,
    i_last_processed_bucket text DEFAULT NULL,
    i_status int DEFAULT NULL
) RETURNS v1_code.lifecycle_scheduling_status LANGUAGE plpgsql AS
$$
DECLARE
    result v1_code.lifecycle_scheduling_status;
BEGIN
    IF i_last_processed_bucket IS NULL AND i_status IS NULL THEN
        RAISE EXCEPTION 'Invalid update_lifecycle_scheduling_status(name = %) arguments, all are NULL', i_key
                        USING ERRCODE = '22004';
    END IF;

    UPDATE s3.lifecycle_scheduling_status
        SET last_processed_bucket = coalesce(i_last_processed_bucket, last_processed_bucket),
            status = coalesce(i_status, status)
        WHERE key = i_key
        RETURNING key, scheduling_ts, last_processed_bucket, status
        INTO result;
    RETURN result;
END;
$$;
