SELECT
    h.subcid,
    h.shard_id,
    sh.name AS shard_name,
    space_limit,
    flavor_id AS flavor,
    geo,
    fqdn,
    assign_public_ip,
    f.name AS flavor_name,
    h.vtype,
    vtype_id,
    disk_type_id,
    subnet_id,
    cast(roles AS text[]) AS roles
FROM code.get_hosts_by_cid(%(cid)s) h
JOIN dbaas.flavors f
  ON f.id = h.flavor_id
LEFT JOIN dbaas.shards sh
  ON sh.shard_id = h.shard_id
WHERE (%(role)s IS NULL OR %(role)s = ANY(roles))
  AND (%(subcid)s IS NULL OR %(subcid)s = h.subcid)
ORDER BY fqdn
