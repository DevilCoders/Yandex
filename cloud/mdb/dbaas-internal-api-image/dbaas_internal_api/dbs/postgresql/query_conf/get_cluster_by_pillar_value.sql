SELECT
    cid,
    subcid,
    shard_id,
    fqdn
FROM
    dbaas.pillar
WHERE
    value @> %(value)s
ORDER BY cid
