SELECT
    c.cid
FROM
    dbaas.clusters c
    JOIN dbaas.subclusters sc USING (cid)
    JOIN dbaas.hosts h USING (subcid)
WHERE
    (%(fqdn)s IS NULL OR h.fqdn = %(fqdn)s)
    AND (%(vtype_id)s IS NULL OR h.vtype_id = %(vtype_id)s)
    AND (%(shard_id)s IS NULL OR h.shard_id = %(shard_id)s)
    AND (%(subcid)s IS NULL OR h.subcid = %(subcid)s)
    AND code.visible(c)
