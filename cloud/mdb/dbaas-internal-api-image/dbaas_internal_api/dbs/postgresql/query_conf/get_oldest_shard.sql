SELECT
    s.subcid,
    s.shard_id,
    s.name,
    coalesce(pl.value, '{}'::jsonb) "value"
FROM dbaas.shards s
JOIN dbaas.subclusters sc USING (subcid)
LEFT JOIN dbaas.pillar pl ON s.shard_id = pl.shard_id
WHERE (sc.cid = %(cid)s)
  AND (%(role)s IS NULL OR %(role)s = ANY(sc.roles))
ORDER BY s.created_at
LIMIT 1;
