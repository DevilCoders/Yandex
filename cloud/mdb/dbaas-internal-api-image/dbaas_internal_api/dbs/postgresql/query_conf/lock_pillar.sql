SELECT
    cid,
    subcid,
    shard_id,
    fqdn,
    value
FROM
    dbaas.pillar
WHERE
    (%(cid)s IS NULL OR cid = %(cid)s)
    AND (%(subcid)s IS NULL OR subcid = %(subcid)s)
    AND (%(shard_id)s IS NULL OR shard_id = %(shard_id)s)
    AND (%(fqdn)s IS NULL OR fqdn = %(fqdn)s)
FOR UPDATE
