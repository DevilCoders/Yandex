CREATE FUNCTION code.get_lock_status(
    i_lock_ext_id text
) RETURNS code.lock_status AS $$
DECLARE
    v_lock        mlock.locks;
    v_conflicts   jsonb[];
    v_acquired    boolean;
    v_objects     text[];
    v_lock_status code.lock_status;
BEGIN
    PERFORM pg_advisory_xact_lock_shared(1);

    SELECT *
    INTO v_lock
    FROM mlock.locks
    WHERE lock_ext_id = i_lock_ext_id;

    IF NOT found THEN
        RAISE EXCEPTION 'Unable to find lock with id %', i_lock_ext_id
        USING ERRCODE = 'MLD02';
    END IF;

    WITH objs AS (
        SELECT o.object_name AS object_name
        FROM
            mlock.object_locks ol
            JOIN mlock.objects o ON (o.object_id = ol.object_id)
        WHERE
            ol.lock_id = v_lock.lock_id
        ORDER BY o.object_name)
    SELECT array_agg(object_name)
    INTO v_objects
    FROM objs;

    WITH conflicts AS (
        SELECT
            sj.object_name AS object,
            array_agg(sj.lock_ext_id) AS lock_ext_ids
        FROM (
            SELECT o.object_name, l.lock_ext_id
            FROM
                mlock.object_locks ol
                JOIN mlock.locks l ON (l.lock_id = ol.lock_id)
                JOIN mlock.objects o ON (o.object_id = ol.object_id)
                JOIN mlock.object_locks sol ON (sol.object_id = ol.object_id)
            WHERE
                ol.contend_order < sol.contend_order
                AND sol.lock_id = v_lock.lock_id
            ORDER BY ol.object_id, ol.contend_order
        ) sj GROUP BY sj.object_name)
    SELECT array_agg(jsonb_build_object('object', object, 'ids', lock_ext_ids))
    INTO v_conflicts
    FROM conflicts;

    IF coalesce(cardinality(v_conflicts), 0) > 0 THEN
        v_acquired := false;
    ELSE
        v_acquired := true;
    END IF;

    SELECT
        v_lock.lock_ext_id AS lock_ext_id,
        v_lock.holder AS holder,
        v_lock.reason AS reason,
        v_lock.create_ts AS create_ts,
        v_objects AS objects,
        v_acquired AS acquired,
        array_to_json(v_conflicts)::jsonb AS conflicts
    INTO v_lock_status;

    RETURN v_lock_status;
END;
$$ LANGUAGE plpgsql;
