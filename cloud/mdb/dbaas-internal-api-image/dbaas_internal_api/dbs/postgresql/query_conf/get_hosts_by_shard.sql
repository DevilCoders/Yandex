SELECT
    h.subcid,
    h.shard_id,
    s.name AS shard_name,
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
    roles::text[] AS roles
FROM code.get_hosts_by_shard(%(shard_id)s) h
JOIN dbaas.flavors f
  ON f.id = h.flavor_id
LEFT JOIN dbaas.shards s
  ON s.shard_id = h.shard_id
ORDER BY fqdn
