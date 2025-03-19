SELECT
    subcid,
    cid,
    name,
    roles::text[],
    created_at
FROM
    dbaas.subclusters
WHERE
    cid = %(cid)s
    AND (%(page_token_subcid)s IS NULL OR subcid > %(page_token_subcid)s)
ORDER BY
    subcid
LIMIT %(limit)s