CREATE OR REPLACE FUNCTION v1_code.poll(
    OUT is_master boolean,
    OUT lag FLOAT,
    OUT sessions_ratio FLOAT
) AS $$
DECLARE
    closed boolean;
BEGIN
    closed := current_setting('pgcheck.closed', true /* missing_ok */);
    IF closed IS true THEN
        RAISE EXCEPTION 'Database is closed from load (pgcheck.closed = %)', closed;
    END IF;

    SELECT NOT pg_is_in_recovery(), 0
          INTO is_master, sessions_ratio;

    IF is_master THEN
        SELECT 0 INTO lag;
    ELSE
        SELECT coalesce(extract(epoch FROM clock_timestamp() - pg_last_xact_replay_timestamp()), 100500)
          INTO lag;
    END IF;
END
$$ LANGUAGE plpgsql SECURITY DEFINER;
