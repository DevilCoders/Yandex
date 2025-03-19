SELECT
    sc.subcid,
    sc.cid,
    sc.name,
    sc.roles::text[],
    coalesce(pl.value, '{}'::jsonb) "value"
FROM
    dbaas.subclusters sc
    LEFT JOIN dbaas.pillar pl ON sc.subcid = pl.subcid
WHERE
    sc.cid = %(cid)s
