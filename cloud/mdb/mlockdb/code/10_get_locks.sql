CREATE FUNCTION code.get_locks(
    i_holder text DEFAULT NULL,
    i_limit  bigint DEFAULT 100,
    i_offset bigint DEFAULT 0
) RETURNS SETOF code.lock AS $$
SELECT
    l.lock_ext_id AS lock_ext_id,
    l.holder AS holder,
    l.reason AS reason,
    l.create_ts AS create_ts,
    array_agg(o.object_name) AS objects
FROM
    mlock.locks l
    JOIN mlock.object_locks ol ON (l.lock_id = ol.lock_id)
    JOIN mlock.objects o ON (ol.object_id = o.object_id)
WHERE
    (i_holder IS NULL OR l.holder = i_holder)
GROUP BY l.lock_id
ORDER BY l.create_ts
LIMIT i_limit
OFFSET i_offset;
$$ LANGUAGE SQL STABLE;
