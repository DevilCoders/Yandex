CREATE FUNCTION code.release_lock(
    i_lock_ext_id text
) RETURNS void AS $$
DECLARE
    v_lock mlock.locks;
BEGIN
    PERFORM pg_advisory_xact_lock(1);

    SELECT *
    INTO v_lock
    FROM mlock.locks
    WHERE lock_ext_id = i_lock_ext_id;

    IF NOT found THEN
        RAISE EXCEPTION 'Unable to find lock with id %', i_lock_ext_id
        USING ERRCODE = 'MLD02';
    END IF;

    DELETE FROM mlock.object_locks
    WHERE lock_id = v_lock.lock_id;

    DELETE FROM mlock.objects
    WHERE object_id IN (
        SELECT o.object_id
        FROM mlock.objects o
        LEFT JOIN mlock.object_locks ol ON (o.object_id = ol.object_id)
        WHERE ol.object_id IS NULL);

    DELETE FROM mlock.locks
    WHERE lock_id = v_lock.lock_id;
END;
$$ LANGUAGE plpgsql;
