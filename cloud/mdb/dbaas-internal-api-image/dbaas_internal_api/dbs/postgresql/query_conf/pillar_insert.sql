UPDATE
    dbaas.pillar
SET
    value = jsonb_insert(value, %(path)s, %(value)s)
WHERE
    (%(cid)s IS NULL OR (cid = %(cid)s AND cid IS NOT NULL))
    AND (%(subcid)s IS NULL OR (subcid = %(subcid)s AND subcid IS NOT NULL))
    AND (%(shard_id)s IS NULL OR (shard_id = %(shard_id)s AND shard_id IS NOT NULL))
    AND (%(fqdn)s IS NULL OR (fqdn = %(fqdn)s AND fqdn IS NOT NULL))
RETURNING
    cid,
    subcid,
    shard_id,
    fqdn
