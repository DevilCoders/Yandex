SELECT
    s.subcid,
    s.shard_id,
    s.name,
    coalesce(pl.value, '{}'::jsonb) "value"
FROM
    dbaas.shards s
    JOIN dbaas.subclusters sc USING (subcid)
    LEFT JOIN dbaas.pillar pl ON s.shard_id = pl.shard_id
WHERE
    (%(cid)s IS NULL OR sc.cid = %(cid)s)
    AND (%(subcid)s IS NULL OR sc.subcid = %(subcid)s)
    AND (%(role)s IS NULL OR %(role)s = ANY(sc.roles))
ORDER BY
    s.name
