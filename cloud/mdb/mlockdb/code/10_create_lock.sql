CREATE FUNCTION code.create_lock(
    i_lock_ext_id text,
    i_holder      text,
    i_reason      text,
    i_objects     text[]
) RETURNS void AS $$
DECLARE
    v_created_lock   mlock.locks;
    v_ts             timestamptz;
    v_objects        text[];
    v_sorted_objects text[];
    v_object_name    text;
    v_object         mlock.objects;
    v_contend_order  bigint;
BEGIN
    ASSERT coalesce(cardinality(i_objects), 0) > 0, 'i_objects should be not empty';

    PERFORM pg_advisory_xact_lock(1);

    v_ts := clock_timestamp();

    SELECT array(SELECT unnest(i_objects) ORDER BY 1) INTO v_sorted_objects;

    INSERT INTO mlock.locks (
        lock_ext_id,
        holder,
        reason,
        create_ts
    ) VALUES (
        i_lock_ext_id,
        i_holder,
        i_reason,
        v_ts
    ) ON CONFLICT ON CONSTRAINT uk_locks_lock_ext_id DO
    UPDATE SET try_count = mlock.locks.try_count + 1
    RETURNING * INTO v_created_lock;

    IF v_created_lock.try_count != 1 THEN
        IF v_created_lock.holder != i_holder THEN
            RAISE EXCEPTION 'Found existing lock % with holder %', i_lock_ext_id, v_created_lock.holder
            USING ERRCODE = 'MLD01';
        END IF;

        IF v_created_lock.reason != i_reason THEN
            RAISE EXCEPTION 'Found existing lock % with reason %', i_lock_ext_id, v_created_lock.reason
            USING ERRCODE = 'MLD01';
        END IF;

        SELECT array_agg(o.object_name ORDER BY o.object_name)
        INTO v_objects
        FROM
            mlock.object_locks ol
            JOIN mlock.objects o ON (o.object_id = ol.object_id)
        WHERE
            ol.lock_id = v_created_lock.lock_id;

        IF v_sorted_objects != v_objects THEN
            RAISE EXCEPTION 'Found existing lock % with objects %', i_lock_ext_id, array_to_string(v_objects, ', ')
            USING ERRCODE = 'MLD01';
        END IF;
    ELSE
        FOREACH v_object_name IN ARRAY v_sorted_objects LOOP
            SELECT *
            INTO v_object
            FROM mlock.objects
            WHERE object_name = v_object_name;

            IF found THEN
                SELECT max(contend_order) + 1
                INTO v_contend_order
                FROM mlock.object_locks
                WHERE object_id = v_object.object_id
                GROUP BY object_id;
            ELSE
                INSERT INTO mlock.objects (
                    object_name
                ) VALUES (
                    v_object_name
                ) RETURNING * INTO v_object;

                v_contend_order := 1;
            END IF;

            INSERT INTO mlock.object_locks (
                lock_id,
                object_id,
                contend_order
            ) VALUES (
                v_created_lock.lock_id,
                v_object.object_id,
                v_contend_order
            );
        END LOOP;
    END IF;
END;
$$ LANGUAGE plpgsql;
