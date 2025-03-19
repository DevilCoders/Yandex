SELECT
    subcid,
    cid,
    name,
    roles::text[],
    created_at
FROM dbaas.subclusters
WHERE subcid = %(subcid)s
