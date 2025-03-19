INSERT INTO dbaas.target_pillar (
    target_id,
    cid,
    subcid,
    shard_id,
    fqdn,
    value
)
VALUES (
    %(target_id)s,
    %(cid)s,
    %(subcid)s,
    %(shard_id)s,
    %(fqdn)s,
    %(value)s
)
RETURNING
    target_id
    cid,
    subcid,
    shard_id,
    fqdn
